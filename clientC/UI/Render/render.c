#include "render.h"
#include "raylib.h"
#include <stdlib.h>   // for abs()


// -----------------------------------------------------------------------------
// Texture storage (loaded once in render_init_assets)
// -----------------------------------------------------------------------------

// Donkey Jr textures
static Texture2D texJrIdle0;
static Texture2D texJrWalk0;
static Texture2D texJrWalk1;
static Texture2D texJrWalk2;
static Texture2D texJrJump;
static Texture2D texJrFall;
static Texture2D texJrClimb0;
static Texture2D texJrClimb1;
static Texture2D texJrBetweenVines;

// Crocodile textures (right-facing)
static Texture2D texCrocBlueClosed;
static Texture2D texCrocBlueOpen;
static Texture2D texCrocRedClosed;
static Texture2D texCrocRedOpen;

// Fruit textures
static Texture2D texFruitApple;
static Texture2D texFruitOrange;
static Texture2D texFruitBanana;

// Simple per-entity animation state (render-side only)
static int crocAnimFrame[MAX_CROCS];    // 0 = closed, 1 = open
static int crocAnimCounter[MAX_CROCS];  // frame counter

static int jrWalkFrame     = 0;  // 0,1,2
static int jrWalkCounter   = 0;
static int jrClimbFrame    = 0;  // 0,1
static int jrClimbCounter  = 0;


static int jrFacing = 1;  // 1 = right, -1 = left

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void set_point_filter(Texture2D tex) {
    if (tex.id != 0) {
        SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    }
}

// -----------------------------------------------------------------------------
// Load all sprite textures
// -----------------------------------------------------------------------------
void render_init_assets(void) {
    
    texJrIdle0        = LoadTexture("clientC/UI/sprites/jr_walk_0.png");
    texJrWalk0        = LoadTexture("clientC/UI/sprites/jr_walk_0.png");
    texJrWalk1        = LoadTexture("clientC/UI/sprites/jr_walk_1.png");
    texJrWalk2        = LoadTexture("clientC/UI/sprites/jr_walk_2.png");
    texJrJump         = LoadTexture("clientC/UI/sprites/jr_jump.png");
    texJrFall         = LoadTexture("clientC/UI/sprites/jr_fall.png");
    texJrClimb0       = LoadTexture("clientC/UI/sprites/jr_climb_0.png");
    texJrClimb1       = LoadTexture("clientC/UI/sprites/jr_climb_1.png");
    texJrBetweenVines = LoadTexture("clientC/UI/sprites/jr_between_vines.png");

    // Crocs
    texCrocBlueClosed = LoadTexture("clientC/UI/sprites/croc_blue_closed.png");
    texCrocBlueOpen   = LoadTexture("clientC/UI/sprites/croc_blue_open.png");
    texCrocRedClosed  = LoadTexture("clientC/UI/sprites/croc_red_closed.png");
    texCrocRedOpen    = LoadTexture("clientC/UI/sprites/croc_red_open.png");

    // Fruits
    texFruitApple     = LoadTexture("clientC/UI/sprites/fruit_apple.png");
    texFruitOrange    = LoadTexture("clientC/UI/sprites/fruit_orange.png");
    texFruitBanana    = LoadTexture("clientC/UI/sprites/fruit_banana.png");

    // Keep pixel-art crisp
    set_point_filter(texJrIdle0);
    set_point_filter(texJrWalk0);
    set_point_filter(texJrWalk1);
    set_point_filter(texJrWalk2);
    set_point_filter(texJrJump);
    set_point_filter(texJrFall);
    set_point_filter(texJrClimb0);
    set_point_filter(texJrClimb1);
    set_point_filter(texJrBetweenVines);

    set_point_filter(texCrocBlueClosed);
    set_point_filter(texCrocBlueOpen);
    set_point_filter(texCrocRedClosed);
    set_point_filter(texCrocRedOpen);

    set_point_filter(texFruitApple);
    set_point_filter(texFruitOrange);
    set_point_filter(texFruitBanana);

    // Reset animation state
    for (int i = 0; i < MAX_CROCS; ++i) {
        crocAnimFrame[i]   = 0;
        crocAnimCounter[i] = 0;
    }

    jrWalkFrame    = 0;
    jrWalkCounter  = 0;
    jrClimbFrame   = 0;
    jrClimbCounter = 0;
}

// -----------------------------------------------------------------------------
// Unload all texture resources
// -----------------------------------------------------------------------------
void render_shutdown_assets(void) {
    UnloadTexture(texJrIdle0);
    UnloadTexture(texJrWalk0);
    UnloadTexture(texJrWalk1);
    UnloadTexture(texJrWalk2);
    UnloadTexture(texJrJump);
    UnloadTexture(texJrFall);
    UnloadTexture(texJrClimb0);
    UnloadTexture(texJrClimb1);
    UnloadTexture(texJrBetweenVines);

    UnloadTexture(texCrocBlueClosed);
    UnloadTexture(texCrocBlueOpen);
    UnloadTexture(texCrocRedClosed);
    UnloadTexture(texCrocRedOpen);

    UnloadTexture(texFruitApple);
    UnloadTexture(texFruitOrange);
    UnloadTexture(texFruitBanana);
}

// -----------------------------------------------------------------------------
// Internal: pick Jr texture based on current "visual state"
// -----------------------------------------------------------------------------
typedef enum {
    JR_VIS_IDLE,
    JR_VIS_WALK,
    JR_VIS_JUMP,
    JR_VIS_FALL,
    JR_VIS_CLIMB,
    JR_VIS_BETWEEN_VINES
} JrVisualState;

static Texture2D choose_jr_texture(const Player* p, JrVisualState* outState) {
    JrVisualState state;

    if (p->onVine) {
        if (p->betweenVines) {
            state = JR_VIS_BETWEEN_VINES;
        } else {
            state = JR_VIS_CLIMB;
        }
    } else if (!p->grounded) {
        if (p->vy < 0) state = JR_VIS_JUMP;
        else           state = JR_VIS_FALL;
    } else {
        if (p->vx != 0) state = JR_VIS_WALK;
        else            state = JR_VIS_IDLE;
    }

    if (outState) *outState = state;

    // Simple animation for walk and climb (frame-based, no dt)
    const int WALK_FRAMES = 3;
    const int CLIMB_FRAMES = 2;

    const int WALK_SPEED = 8;   // lower = faster switching
    const int CLIMB_SPEED = 10;

    switch (state) {
        case JR_VIS_WALK:
            jrWalkCounter++;
            if (jrWalkCounter >= WALK_SPEED) {
                jrWalkCounter = 0;
                jrWalkFrame = (jrWalkFrame + 1) % WALK_FRAMES;
            }
            switch (jrWalkFrame) {
                case 0: return texJrWalk0;
                case 1: return texJrWalk1;
                default:return texJrWalk2;
            }

        case JR_VIS_CLIMB:
            jrClimbCounter++;
            if (jrClimbCounter >= CLIMB_SPEED) {
                jrClimbCounter = 0;
                jrClimbFrame = (jrClimbFrame + 1) % CLIMB_FRAMES;
            }
            return (jrClimbFrame == 0) ? texJrClimb0 : texJrClimb1;

        case JR_VIS_JUMP:
            // Reset walk animation when leaving ground
            jrWalkFrame   = 0;
            jrWalkCounter = 0;
            return texJrJump;

        case JR_VIS_FALL:
            return texJrFall;

        case JR_VIS_BETWEEN_VINES:
            return texJrBetweenVines;

        case JR_VIS_IDLE:
        default:
            jrWalkFrame   = 0;
            jrWalkCounter = 0;
            return texJrIdle0;
    }
}

// -----------------------------------------------------------------------------
// Draw static level + all dynamic entities
// -----------------------------------------------------------------------------
void render_draw_level(const struct CP_Static* staticMap,
                       const Player*    player,
                       const Crocodile* crocs,  int crocCount,
                       const Fruit*     fruits, int fruitCount)
{
    if (!staticMap) return;

    // -------------------------------------------------------------------------
    // Draw water (filled)
    // -------------------------------------------------------------------------
    for (int i = 0; i < staticMap->nWater; i++) {
        DrawRectangle(staticMap->water[i].x, staticMap->water[i].y,
                      staticMap->water[i].w, staticMap->water[i].h,
                      SKYBLUE);
    }

    // -------------------------------------------------------------------------
    // Draw platforms (outline)
    // -------------------------------------------------------------------------
    for (int i = 0; i < staticMap->nPlat; i++) {
        DrawRectangleLines(staticMap->plat[i].x, staticMap->plat[i].y,
                           staticMap->plat[i].w, staticMap->plat[i].h,
                           GREEN);
    }

    // -------------------------------------------------------------------------
    // Draw vines (outline)
    // -------------------------------------------------------------------------
    for (int i = 0; i < staticMap->nVines; i++) {
        DrawRectangleLines(staticMap->vines[i].x, staticMap->vines[i].y,
                           staticMap->vines[i].w, staticMap->vines[i].h,
                           YELLOW);
    }

        // -------------------------------------------------------------------------
    // Draw Player sprite + hitbox debug
    // -------------------------------------------------------------------------
    if (player) {
        JrVisualState st;
        Texture2D jrTex = choose_jr_texture(player, &st);

        // Start from last facing so idle / jump / fall keep direction
        int facing = jrFacing;  // 1 = right, -1 = left

        // 1) Update facing from horizontal movement
        if (player->vx > 0)      facing =  1;
        else if (player->vx < 0) facing = -1;

        // 2) If on a vine, refine facing using the nearest vine
        if (player->onVine && staticMap) {
            // Player center
            float pxCenter = player->x + player->w * 0.5f;
            float pyCenter = player->y + player->h * 0.5f;

            bool  found    = false;
            float bestDist = 0.0f;
            float bestVineCenterX = 0.0f;

            for (int i = 0; i < staticMap->nVines; ++i) {
                float vx = (float)staticMap->vines[i].x;
                float vy = (float)staticMap->vines[i].y;
                float vw = (float)staticMap->vines[i].w;
                float vh = (float)staticMap->vines[i].h;

                // Only consider vines that overlap vertically with Jr
                bool overlapsY =
                    (pyCenter >= vy) &&
                    (pyCenter <= vy + vh);
                if (!overlapsY) continue;

                float vineCenterX = vx + vw * 0.5f;
                float dx = pxCenter - vineCenterX;
                float dist = (dx < 0.0f) ? -dx : dx;  // |dx| sin usar fabsf

                if (!found || dist < bestDist) {
                    found = true;
                    bestDist = dist;
                    bestVineCenterX = vineCenterX;
                }
            }

            if (found) {
                // If player center is left of the vine → facing right
                // If player center is right of the vine → facing left
                if (pxCenter < bestVineCenterX)
                    facing = 1;
                else
                    facing = -1;
            }
        }

        // Store facing for next frame (for idle / in between vines, etc.)
        jrFacing = facing;

        Rectangle src = {
            0.0f,
            0.0f,
            (float)jrTex.width,
            (float)jrTex.height
        };

        if (facing < 0) {
            // Flip horizontally by using negative width
            src.x     = (float)jrTex.width;
            src.width = -src.width;
        }

        Rectangle dst = {
            (float)player->x,
            (float)player->y,
            (float)player->w,
            (float)player->h
        };

        DrawTexturePro(jrTex, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

        // Debug: draw player's hitbox
        DrawRectangleLines(player->x, player->y, player->w, player->h, ORANGE);
    }

    // -------------------------------------------------------------------------
    // Draw Crocodiles (sprite + hitbox)
    // -------------------------------------------------------------------------
    for (int i = 0; i < crocCount && i < MAX_CROCS; ++i) {
        if (!crocs[i].active) continue;

        // Simple open/close mouth animation
        const int CROC_ANIM_SPEED = 12;
        crocAnimCounter[i]++;
        if (crocAnimCounter[i] >= CROC_ANIM_SPEED) {
            crocAnimCounter[i] = 0;
            crocAnimFrame[i]   = 1 - crocAnimFrame[i];
        }

        // Choose closed/open texture based on variant + frame
        Texture2D sprite;
        if (crocs[i].variant == CROC_VARIANT_BLUE) {
            sprite = (crocAnimFrame[i] == 0)
                     ? texCrocBlueClosed
                     : texCrocBlueOpen;
        } else {
            sprite = (crocAnimFrame[i] == 0)
                     ? texCrocRedClosed
                     : texCrocRedOpen;
        }

        // Direction and rotation from velocity
        int vx = crocs[i].vx;
        int vy = crocs[i].vy;
        float angle = 0.0f;
        bool flipX = false;

        if (vx != 0 || vy != 0) {
            if (abs(vx) >= abs(vy)) {
                // Horizontal movement
                if (vx < 0) flipX = true; // looking left
            } else {
                // Vertical movement
                if (vy > 0)      angle = 90.0f;   // down
                else if (vy < 0) angle = -90.0f;  // up
            }
        }

        Rectangle src = {
            0.0f,
            0.0f,
            (float)sprite.width,
            (float)sprite.height
        };

        if (flipX) {
            src.x     = (float)sprite.width;
            src.width = -src.width;
        }

        // Destination rect centered at croc hitbox
        Rectangle dst = {
            (float)(crocs[i].x + crocs[i].w * 0.5f),
            (float)(crocs[i].y + crocs[i].h * 0.5f),
            (float)crocs[i].w,
            (float)crocs[i].h
        };

        // Rotate around the center so the sprite stays aligned with its hitbox
        Vector2 origin = {
            dst.width * 0.5f,
            dst.height * 0.5f
        };

        DrawTexturePro(sprite, src, dst, origin, angle, WHITE);

        // Debug: draw croc hitbox
        DrawRectangleLines(crocs[i].x, crocs[i].y, crocs[i].w, crocs[i].h,
                           (crocs[i].variant == CROC_VARIANT_BLUE) ? BLUE : RED);
    }

    // -------------------------------------------------------------------------
    // Draw Fruits (sprite + hitbox)
    // -------------------------------------------------------------------------
    for (int i = 0; i < fruitCount; ++i) {
        if (!fruits[i].active) continue;

        Texture2D sprite = texFruitBanana;
        if      (fruits[i].variant == FRUIT_VARIANT_APPLE)  sprite = texFruitApple;
        else if (fruits[i].variant == FRUIT_VARIANT_ORANGE) sprite = texFruitOrange;
        // FRUIT_VARIANT_BANANA uses default

        Rectangle src = {
            0.0f,
            0.0f,
            (float)sprite.width,
            (float)sprite.height
        };

        Rectangle dst = {
            (float)fruits[i].x,
            (float)fruits[i].y,
            (float)fruits[i].w,
            (float)fruits[i].h
        };

        DrawTexturePro(sprite, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

        // Debug: draw fruit hitbox
        DrawRectangleLines(fruits[i].x, fruits[i].y, fruits[i].w, fruits[i].h, GOLD);
    }
}
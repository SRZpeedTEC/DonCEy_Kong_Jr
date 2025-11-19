#include "game.h"
#include "static_map.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>



#include "../Render/render.h"
#include "Logic/input.h"
#include "Logic/player.h"
#include "Logic/physics.h"
#include "Logic/map.h"
#include "Logic/crocodile.h"
#include "Logic/constants.h"
#include "Logic/fruit.h"
#include "Logic/collision.h"
#include "../../UtilsC/entities_tlv.h" 



// window/state
static int VW, VH, SCALE;
static RenderTexture2D rt;
static Texture2D g_bg = {0};
static float g_bgAlpha = 0.35f;

// player state
static Player gPlayer;

// crocodile state
static Crocodile gCrocs[MAX_CROCS];

// fruits state
static Fruit gFruits[MAX_FRUITS];

// --- init / shutdown ---
void game_init(uint16_t vw, uint16_t vh, uint16_t scale) {
    VW = (int)vw; VH = (int)vh; SCALE = (int)scale;
    InitWindow(VW * SCALE, VH * SCALE, "Client");
    SetTargetFPS(60);

    rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT); // crisp pixel-art

    // start player box (will be replaced by sprites later)
    player_init(&gPlayer, 16, 192, 16, 16);

    // init croc state
    for (int i = 0; i < MAX_CROCS; ++i) {
        crocodile_init(&gCrocs[i], 8, 8);
    }

    // init fruit state
    for (int i = 0; i < MAX_FRUITS; i++){
        fruit_init(&gFruits[i], 8,8);
    }
}

void game_shutdown(void) {
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }
    UnloadRenderTexture(rt);
    CloseWindow();
}

void game_set_bg(const char* path, float alpha) {
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }
    if (path && *path) {
        g_bg = LoadTexture(path);
        SetTextureFilter(g_bg, TEXTURE_FILTER_POINT);
    }
    if (alpha >= 0.f && alpha <= 1.f) g_bgAlpha = alpha;
}

// --- draw ---
void game_draw_static(const CP_Static* staticMap) {
    BeginTextureMode(rt);
        ClearBackground(BLACK);

        // draw background texture (kept here to avoid exposing Texture2D in render.h)
        if (g_bg.id) {
            Rectangle src = (Rectangle){0, 0, (float)g_bg.width, (float)g_bg.height};
            Rectangle dst = (Rectangle){0, 0, (float)VW, (float)VH};
            DrawTexturePro(g_bg, src, dst, (Vector2){0, 0}, 0, Fade(WHITE, g_bgAlpha));
        }

        // draw level primitives + player box via render module
        render_draw_level(staticMap, gPlayer.x, gPlayer.y, gPlayer.w, gPlayer.h);

        
        // draw all active crocodiles
        for (int i = 0; i < MAX_CROCS; ++i) {
            if (gCrocs[i].active) {
                Color col = (gCrocs[i].variant == CROC_VARIANT_BLUE) ? BLUE : RED;
                DrawRectangle(gCrocs[i].x, gCrocs[i].y, gCrocs[i].w, gCrocs[i].h, col);
            }
        }
        
        // draw all active fruits
        for (int i = 0; i < MAX_FRUITS; ++i) {
            if (gFruits[i].active) {
                Color col = YELLOW;
                if      (gFruits[i].variant == FRUIT_VARIANT_APPLE)  col = PINK;
                else if (gFruits[i].variant == FRUIT_VARIANT_ORANGE) col = ORANGE;
                else                                                 col = YELLOW; // BANANA
                DrawRectangle(gFruits[i].x, gFruits[i].y, gFruits[i].w, gFruits[i].h, col);
            }
        }

    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        // note: negative height to flip the render texture
        Rectangle src = (Rectangle){0, 0, (float)rt.texture.width, -(float)rt.texture.height};
        Rectangle dst = (Rectangle){0, 0, (float)VW * SCALE, (float)VH * SCALE};
        DrawTexturePro(rt.texture, src, dst, (Vector2){0, 0}, 0, WHITE);

            // debug: show if player is on a vine
        MapView debugView = map_view_build();
        bool betweenVines = player_between_vines(&gPlayer, &debugView);

        DrawText(gPlayer.onVine ? "ON VINE" : "NOT ON VINE", 8, 24, 10, WHITE);
        if (gPlayer.betweenVines) {
            DrawText("BETWEEN VINES", 8, 36, 10, YELLOW);
            char buf[64];
            snprintf(buf, sizeof(buf), "LEFT:%d RIGHT:%d", gPlayer.vineLeftIndex, gPlayer.vineRightIndex);
            DrawText(buf, 8, 48, 10, WHITE);
        }
        if (gPlayer.vineForcedFall) {
        DrawText("FORCED FALL", 8, 48, 10, SKYBLUE);
        }

        // death debug
        if (gPlayer.isDead) {
            DrawText("DEAD", 8, 72, 16, RED);
        }

        DrawFPS(8, 8);
    EndDrawing();
}

// --- game logic hook ---
void game_update_and_get_proposal(const CP_Static* staticMap, ProposedState* out) {
    // read keyboard/game input
    InputState in = input_read();

    // simple debug respawn: press R to reset jr at spawn
    if (IsKeyPressed(KEY_R)) {
        game_respawn_player();   // esta función llama internamente a player_init(...)
    }

    // build world view (map data + bounds)
    MapView mv = map_view_build();

    // run physics only if player is alive
    if (!player_is_dead(&gPlayer)) {
        physics_step(&gPlayer, &in, &mv, GetFrameTime());
    } else {
        // dead: freeze velocity so we do not drift
        gPlayer.vx = 0;
        gPlayer.vy = 0;
    }

    // fill proposal to send to server
    out->x = gPlayer.x;
    out->y = gPlayer.y;
    out->vx = gPlayer.vx;
    out->vy = gPlayer.vy;

    // flags:
    // bit 0 -> grounded (on floor or platform)
    // bit 1 -> just died this frame (one–shot event)
    uint8_t flags = 0;
    if (gPlayer.grounded) {
        flags |= 0x01;
    }
    if (player_just_died(&gPlayer)) {
        flags |= 0x02;
    }
    out->flags = flags;

    (void)staticMap; // not used yet on the client logic side
}

// returns the total size written to dst, or 0 if dst is NULL / too small.
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity) {
    if (!dst || dstCapacity == 0) return 0;

    // temporary buffer to collect all entities this frame
    EntitySnapshot snapshots[1 + MAX_CROCS + MAX_FRUITS];
    uint8_t count = 0;

    // --- player snapshot ---
    {
        uint8_t spriteId = 0;  // placeholder, later replaced by real animation logic
        snapshots[count].kind     = ENTITY_KIND_PLAYER;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gPlayer.x;
        snapshots[count].y        = gPlayer.y;
        count++;
    }

    // --- crocodile snapshots ---
    for (int i = 0; i < MAX_CROCS; ++i) {
        if (!gCrocs[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        // simple variant → sprite mapping
        if      (gCrocs[i].variant == CROC_VARIANT_BLUE) spriteId = 1;
        else if (gCrocs[i].variant == CROC_VARIANT_RED)  spriteId = 2;

        snapshots[count].kind     = ENTITY_KIND_CROC;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gCrocs[i].x;
        snapshots[count].y        = gCrocs[i].y;
        count++;
    }

    // --- fruit snapshots ---
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        if      (gFruits[i].variant == FRUIT_VARIANT_APPLE)  spriteId = 1;
        else if (gFruits[i].variant == FRUIT_VARIANT_ORANGE) spriteId = 2;
        else if (gFruits[i].variant == FRUIT_VARIANT_BANANA) spriteId = 3;

        snapshots[count].kind     = ENTITY_KIND_FRUIT;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gFruits[i].x;
        snapshots[count].y        = gFruits[i].y;
        count++;
    }

    // serialize all collected snapshots into a TLV buffer
    return entities_tlv_build(dst, dstCapacity, snapshots, count);
}


// build a TLV_ENTITIES_CORR containing: player, all active crocs and all active fruits.
// returns the total size written to dst, or 0 if dst is NULL / too small.
size_t game_build_entities_tlv(uint8_t* dst, size_t dstCapacity) {
    if (!dst || dstCapacity == 0) return 0;

    // temporary buffer to collect all entities this frame
    EntitySnapshot snapshots[1 + MAX_CROCS + MAX_FRUITS];
    uint8_t count = 0;

    // --- player snapshot ---
    {
        uint8_t spriteId = 0;  // placeholder, later replaced by real animation logic
        snapshots[count].kind     = ENTITY_KIND_PLAYER;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gPlayer.x;
        snapshots[count].y        = gPlayer.y;
        count++;
    }

    // --- crocodile snapshots ---
    for (int i = 0; i < MAX_CROCS; ++i) {
        if (!gCrocs[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        // simple variant → sprite mapping
        if      (gCrocs[i].variant == CROC_VARIANT_BLUE) spriteId = 1;
        else if (gCrocs[i].variant == CROC_VARIANT_RED)  spriteId = 2;

        snapshots[count].kind     = ENTITY_KIND_CROC;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gCrocs[i].x;
        snapshots[count].y        = gCrocs[i].y;
        count++;
    }

    // --- fruit snapshots ---
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) continue;
        if (count >= sizeof(snapshots) / sizeof(snapshots[0])) break;

        uint8_t spriteId = 0;
        if      (gFruits[i].variant == FRUIT_VARIANT_APPLE)  spriteId = 1;
        else if (gFruits[i].variant == FRUIT_VARIANT_ORANGE) spriteId = 2;
        else if (gFruits[i].variant == FRUIT_VARIANT_BANANA) spriteId = 3;

        snapshots[count].kind     = ENTITY_KIND_FRUIT;
        snapshots[count].spriteId = spriteId;
        snapshots[count].x        = gFruits[i].x;
        snapshots[count].y        = gFruits[i].y;
        count++;
    }

    // serialize all collected snapshots into a TLV buffer
    return entities_tlv_build(dst, dstCapacity, snapshots, count);
}



void game_apply_correction(uint32_t tick, uint8_t grounded, int16_t platId, int16_t yCorr, int16_t vyCorr) {
    (void)tick; (void)platId;

    // snap or nudge according to server correction
    if (grounded) {
        gPlayer.y = yCorr;
        gPlayer.vy = 0;
        gPlayer.grounded = true;
    } else {
        gPlayer.y += vyCorr;
        gPlayer.grounded = false;
    }
}

void game_apply_remote_state(int16_t x, int16_t y, int16_t vx, int16_t vy, uint8_t flags) {
    // Sobrescribe la posición y velocidad del jugador con lo que manda el server
    gPlayer.x  = x;
    gPlayer.y  = y;
    gPlayer.vx = vx;
    gPlayer.vy = vy;

    // Flags vienen con el mismo significado que en game_update_and_get_proposal:
    // bit 0 -> grounded
    // bit 1 -> just died
    gPlayer.grounded = (flags & 0x01) != 0;

    if (flags & 0x02) {
        // Si quieres reflejar la muerte en pantalla:
        gPlayer.isDead = true;
    }
}

void game_spawn_croc(uint8_t variant, int16_t x, int16_t y) {
    // search for an inactive crocodile to spawn
    for (int i = 0; i < MAX_CROCS; ++i) {
        if (!gCrocs[i].active) {
            crocodile_spawn(&gCrocs[i], variant, x, y);
            return;
        }
    }
    // if everything is active, overwrite the first one
    crocodile_spawn(&gCrocs[0], variant, x, y);
}

void game_spawn_fruit(uint8_t variant, int16_t x, int16_t y) {
    // search for an inactive fruit to spawn
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) {
            fruit_spawn(&gFruits[i], variant, x, y);
            return;
        }
    }
    // if everything is active, overwrite the first one
    fruit_spawn(&gFruits[0], variant, x, y);
}


void game_remove_fruit_at(int16_t x, int16_t y){
    for (int i = 0; i < MAX_FRUITS; ++i) {
        if (!gFruits[i].active) continue;
        int16_t fx = gFruits[i].x, fy = gFruits[i].y, fw = gFruits[i].w, fh = gFruits[i].h;
        if (x >= fx && x < fx + fw && y >= fy && y < fy + fh) {
            gFruits[i].active = false;
            break;
        }
    }
}

void game_respawn_player(void) {
    // respawn on initial platform 
    int startX = 16;
    int startY = 192;

    // maintain same rectangle size used originally
    player_init(&gPlayer, startX, startY, gPlayer.w, gPlayer.h);
}




#include "game.h"
#include "static_map.h"
#include "raylib.h"
#include <string.h>

#include "../Render/render.h"
#include "Logic/input.h"
#include "Logic/player.h"
#include "Logic/physics.h"
#include "Logic/map.h"
#include "Logic/crocodile.h"
#include "Logic/constants.h"
#include "Logic/fruit.h"

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
        DrawText(gPlayer.onVine ? "ON VINE" : "NOT ON VINE", 8, 24, 10, WHITE);

        DrawFPS(8, 8);
    EndDrawing();
}

// --- game logic hook ---
void game_update_and_get_proposal(const CP_Static* staticMap, ProposedState* out) {
    // read intents
    InputState in = input_read();

    // world view (bounds come from here)
    MapView mv = map_view_build();

    // apply simple physics (gravity, jump, bounds)
    physics_step(&gPlayer, &in, &mv, GetFrameTime());

    // build proposal for the server
    out->x = gPlayer.x;
    out->y = gPlayer.y;
    out->vx = gPlayer.vx;
    out->vy = gPlayer.vy;
    out->flags = gPlayer.grounded ? 1 : 0;

    (void)staticMap; // not needed here yet
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

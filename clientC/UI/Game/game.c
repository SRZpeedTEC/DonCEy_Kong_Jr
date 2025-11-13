#include "game.h"
#include "static_map.h"
#include "raylib.h"
#include <string.h>

#include "../Render/render.h"
#include "Logic/input.h"
#include "Logic/player.h"
#include "Logic/physics.h"
#include "Logic/map.h"

// window/state
static int VW, VH, SCALE;
static RenderTexture2D rt;
static Texture2D g_bg = {0};
static float g_bgAlpha = 0.35f;

// player state
static Player gPlayer;

// crocodile constants
static bool gCrocActive = false;
static int  gCrocX;
static int  gCrocY;
static int  gCrocW;
static int  gCrocH;

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
    gCrocActive = false;
    gCrocX = gCrocY = 0;
    gCrocW = gCrocH = 8;
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

        // draw crocodile square (if active)
        if (gCrocActive) {
            DrawRectangleLines(gCrocX, gCrocY, gCrocW, gCrocH, RED);
        }
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        // note: negative height to flip the render texture
        Rectangle src = (Rectangle){0, 0, (float)rt.texture.width, -(float)rt.texture.height};
        Rectangle dst = (Rectangle){0, 0, (float)VW * SCALE, (float)VH * SCALE};
        DrawTexturePro(rt.texture, src, dst, (Vector2){0, 0}, 0, WHITE);
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

void game_spawn_croc(int16_t x, int16_t y) {
    gCrocX = x;
    gCrocY = y;
    gCrocActive = true;
}
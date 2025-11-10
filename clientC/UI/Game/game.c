#include "game.h"
#include "static_map.h"
#include "raylib.h"
#include <string.h>

// ===== estado de render =====
static int VW, VH, SCALE;
static RenderTexture2D rt;
static Texture2D g_bg = {0};
static float g_bgAlpha = 0.35f;

// (jugador de ejemplo: ajusta cuando uses tu estado real)
typedef struct { int16_t x,y,w,h; } PlayerBox;
static PlayerBox G = {16,192,16,16};

// ===== init / shutdown =====
void game_init(uint16_t vw, uint16_t vh, uint16_t scale){
    VW = (int)vw; VH = (int)vh; SCALE = (int)scale;
    InitWindow(VW*SCALE, VH*SCALE, "Client");
    SetTargetFPS(60);

    rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT); // pixel-art nítido
}

void game_shutdown(void){
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }
    UnloadRenderTexture(rt);
    CloseWindow();
}

void game_set_bg(const char* path, float alpha){
    if (g_bg.id) { UnloadTexture(g_bg); g_bg.id = 0; }
    if (path && *path){
        g_bg = LoadTexture(path);
        SetTextureFilter(g_bg, TEXTURE_FILTER_POINT);
    }
    if (alpha>=0.f && alpha<=1.f) g_bgAlpha = alpha;
}

// ===== dibujo =====
static void draw_level(const CP_Static* s){
    // fondo
    if (g_bg.id){
        Rectangle src = (Rectangle){0,0,(float)g_bg.width,(float)g_bg.height};
        Rectangle dst = (Rectangle){0,0,(float)VW,(float)VH};
        DrawTexturePro(g_bg, src, dst, (Vector2){0,0}, 0, Fade(WHITE, g_bgAlpha));
    }

    // agua (relleno celeste)
    for (int i=0;i<s->nWater;i++)
        DrawRectangle(s->water[i].x, s->water[i].y, s->water[i].w, s->water[i].h, SKYBLUE);

    // plataformas (contorno verde)
    for (int i=0;i<s->nPlat;i++)
        DrawRectangleLines(s->plat[i].x, s->plat[i].y, s->plat[i].w, s->plat[i].h, GREEN);

    // lianas (contorno amarillo)
    for (int i=0;i<s->nVines;i++)
        DrawRectangleLines(s->vines[i].x, s->vines[i].y, s->vines[i].w, s->vines[i].h, YELLOW);

    // jugador (contorno naranja)
    DrawRectangleLines(G.x, G.y, G.w, G.h, ORANGE);
}

void game_draw_static(const CP_Static* s){
    BeginTextureMode(rt);
        ClearBackground(BLACK);
        if (s) draw_level(s);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        // ojo: height negativo para invertir el RT
        Rectangle src = (Rectangle){0,0,(float)rt.texture.width,-(float)rt.texture.height};
        Rectangle dst = (Rectangle){0,0,(float)VW*SCALE,(float)VH*SCALE};
        DrawTexturePro(rt.texture, src, dst, (Vector2){0,0}, 0, WHITE);
        DrawFPS(8,8);
    EndDrawing();
}

// ===== lógica mínima para integrarse con tu loop =====
void game_update_and_get_proposal(const CP_Static* s, ProposedState* out){
    (void)s;
    // aquí va tu movimiento local; por ahora deja quieto
    out->x = G.x; out->y = G.y; out->vx = 0; out->vy = 0; out->flags = 0;
}

void game_apply_correction(uint32_t tick, uint8_t grounded, int16_t platId, int16_t yCorr, int16_t vyCorr){
    (void)tick; (void)platId;
    if (grounded){ G.y = yCorr; }
    else         { G.y += vyCorr; }
}

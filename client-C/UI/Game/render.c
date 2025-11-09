#include "render.h"
#include "raylib.h"

static int VW, VH, SCALE;
static RenderTexture2D rt;

static Texture2D g_bg = (Texture2D){0};
static float g_bgAlpha = 0.35f;

void render_init(int vw, int vh, int scale){
    VW = vw; VH = vh; SCALE = scale;
    InitWindow(VW*SCALE, VH*SCALE, "DK Rects");
    SetTargetFPS(60);
    rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT);
}

void render_shutdown(void){
    render_clear_bg();
    UnloadRenderTexture(rt);
    CloseWindow();
}

void render_frame_static(const CP_Static* s){
    BeginTextureMode(rt);
        if (g_bg.id){
            Rectangle src = (Rectangle){0,0,(float)g_bg.width,(float)g_bg.height};
            Rectangle dst = (Rectangle){0,0,(float)VW,(float)VH};
            DrawTexturePro(g_bg, src, dst, (Vector2){0,0}, 0, Fade(WHITE, g_bgAlpha));
        }
        for (int i=0;i<s->nWater;i++)
            DrawRectangle(s->water[i].x, s->water[i].y, s->water[i].w, s->water[i].h, SKYBLUE);
        for (int i=0;i<s->nPlat;i++)
            DrawRectangleLines(s->plat[i].x, s->plat[i].y, s->plat[i].w, s->plat[i].h, GREEN);
        for (int i=0;i<s->nVines;i++)
            DrawRectangleLines(s->vines[i].x, s->vines[i].y, s->vines[i].w, s->vines[i].h, YELLOW);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK);
        Rectangle src = (Rectangle){0,0,(float)rt.texture.width,-(float)rt.texture.height};
        Rectangle dst = (Rectangle){0,0,(float)VW*SCALE,(float)VH*SCALE};
        DrawTexturePro(rt.texture, src, dst, (Vector2){0,0}, 0, WHITE);
    EndDrawing();
}

void render_set_bg(const char *path, float alpha){
    if (g_bg.id) UnloadTexture(g_bg);
    g_bg = LoadTexture(path);
    SetTextureFilter(g_bg, TEXTURE_FILTER_POINT);
    if (alpha >= 0.0f && alpha <= 1.0f) g_bgAlpha = alpha;
}

void render_clear_bg(void){
    if (g_bg.id){ UnloadTexture(g_bg); g_bg.id = 0; }
}

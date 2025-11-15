// Render module implementation (empty skeleton for now).

#include "render.h"
#include "raylib.h"


// Draw water, platforms, vines and the player box.
void render_draw_level(const struct CP_Static* staticMap
,
                       int playerX, int playerY, int playerW, int playerH)
{
    if (!staticMap) return;

    // water (blue filled)
    for (int i = 0; i < staticMap->nWater; i++) {
        DrawRectangle(staticMap->water[i].x, staticMap->water[i].y,
                      staticMap->water[i].w, staticMap->water[i].h, SKYBLUE);
    }

    // platforms (green outline)
    for (int i = 0; i < staticMap->nPlat; i++) {
        DrawRectangleLines(staticMap->plat[i].x, staticMap->plat[i].y,
                           staticMap->plat[i].w, staticMap->plat[i].h, GREEN);
    }

    // vines (yellow outline)
    for (int i = 0; i < staticMap->nVines; i++) {
        DrawRectangleLines(staticMap->vines[i].x, staticMap->vines[i].y,
                           staticMap->vines[i].w, staticMap->vines[i].h, YELLOW);
    }

    // player (orange outline)
    DrawRectangleLines(playerX, playerY, playerW, playerH, ORANGE);
}
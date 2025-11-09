// mainPlayer.c
// Client "main": connects, reads ACK, pulls initial MATRIX_STATE,
// then sends simple inputs (L/R/U/D/J) and prints updated matrices.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>   // strchr
#include <ctype.h>    // toupper

#include "clientPlayer.h"
#include "net.h"
#include "raylib.h"



static void DrawGeomRaylib(const CP_Geom* g){
    DrawRectangleLines(g->player.x, g->player.y, g->player.w, g->player.h, GREEN);
    for (int i=0;i<g->nPlat;i++)
        DrawRectangleLines(g->plat[i].x, g->plat[i].y, g->plat[i].w, g->plat[i].h, SKYBLUE);
    for (int i=0;i<g->nVines;i++)
        DrawRectangleLines(g->vines[i].x, g->vines[i].y, g->vines[i].w, g->vines[i].h, YELLOW);
    for (int i=0;i<g->nEnemies;i++)
        DrawRectangleLines(g->enemies[i].x, g->enemies[i].y, g->enemies[i].w, g->enemies[i].h, RED);
    for (int i=0;i<g->nFruits;i++)
        DrawRectangleLines(g->fruits[i].x, g->fruits[i].y, g->fruits[i].w, g->fruits[i].h, ORANGE);
}



int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]); return 1; }

    if (!net_init()) { fprintf(stderr, "Network init failed\n"); return 1; }
    int sock = net_connect(argv[1], (uint16_t)atoi(argv[2]));
    if (sock < 0) { net_cleanup(); return 1; }

    // ACK
    struct CP_Header h;
    if (!cp_read_header(sock, &h) || h.version!=CP_VERSION) { fprintf(stderr,"Bad ACK\n"); return 1; }
    if (h.payloadLen) { uint8_t *skip = malloc(h.payloadLen); net_read_n(sock, skip, h.payloadLen); free(skip); }

    // INIT_GEOM
    if (!cp_read_header(sock, &h) || h.type!=CP_TYPE_INIT_GEOM) {
        fprintf(stderr,"Expected INIT_GEOM\n"); return 1;
    }
    uint8_t *payload = (uint8_t*)malloc(h.payloadLen);
    if (!payload || net_read_n(sock, payload, h.payloadLen) <= 0) { fprintf(stderr,"Read fail\n"); return 1; }
    if (!cp_recv_init_geom_sections(payload, h.payloadLen)) { fprintf(stderr,"Bad INIT_GEOM\n"); return 1; }
    free(payload);

    // Raylib window (escala entera sobre 256x240)
    const int VW=256, VH=240, SCALE=3;
    InitWindow(VW*SCALE, VH*SCALE, "DK Rects");
    SetTargetFPS(60);

    // RenderTexture para pixel-perfect (opcional)
    RenderTexture2D rt = LoadRenderTexture(VW, VH);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT); // sin blur

    while (!WindowShouldClose()){
        // (Opcional) leer mensajes no bloqueantes aquí si luego agregamos STATE

        BeginTextureMode(rt);
            ClearBackground((Color){20,20,20,255});
            DrawGeomRaylib(&g_geom);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            // dibuja rt escalado entero y centrado
            Rectangle src = {0,0,(float)rt.texture.width,-(float)rt.texture.height};
            Rectangle dst = {0,0,(float)VW*SCALE,(float)VH*SCALE};
            DrawTexturePro(rt.texture, src, dst, (Vector2){0,0}, 0, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(rt);
    CloseWindow();
    net_close(sock);
    net_cleanup();
    return 0;
}


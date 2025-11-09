#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "proto.h"
#include "net.h"
#include "../UI/Game/render.h"
#include "raylib.h"

int main(int argc, char **argv){
    if (argc < 3){ fprintf(stderr,"Usage: %s <server_ip> <port>\n", argv[0]); return 1; }

    if (!net_init()){ fprintf(stderr,"WSA init fail\n"); return 1; }
    int sock = net_connect(argv[1], (uint16_t)atoi(argv[2]));
    if (sock < 0){ net_cleanup(); return 1; }

    CP_Header h;
    if (!cp_read_header(sock, &h) || h.version!=CP_VERSION || h.type!=CP_TYPE_CLIENT_ACK){
        fprintf(stderr,"Bad ACK\n"); net_close(sock); net_cleanup(); return 1;
    }
    if (h.payloadLen){ uint8_t *skip=(uint8_t*)malloc(h.payloadLen); net_read_n(sock, skip, h.payloadLen); free(skip); }

    if (!cp_read_header(sock, &h) || h.type!=CP_TYPE_INIT_GEOM){
        fprintf(stderr,"Expected INIT_GEOM\n"); net_close(sock); net_cleanup(); return 1;
    }
    uint8_t *payload = (uint8_t*)malloc(h.payloadLen);
    if (!payload || net_read_n(sock, payload, h.payloadLen) <= 0){
        fprintf(stderr,"Read INIT_GEOM fail\n"); free(payload); net_close(sock); net_cleanup(); return 1;
    }
    if (!cp_recv_init_static_payload(payload, h.payloadLen)){
        fprintf(stderr,"Bad INIT_GEOM payload\n"); free(payload); net_close(sock); net_cleanup(); return 1;
    }
    free(payload);

    render_init(256, 240, 3);
    render_set_bg("client-C/UI/Sprites/FONDO1.png", 0.40f);
    while (!WindowShouldClose()){
        render_frame_static(cp_get_static());
    }
    render_shutdown();

    cp_free_static();
    net_close(sock);
    net_cleanup();
    return 0;
}

// clientPlayer.c — portable client using net.*
// Build with net_posix.c on macOS/Linux/WSL, or net_win32.c on Windows.

#include "clientPlayer.h"
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "raylib.h"

CP_Geom g_geom = (CP_Geom){0};

// endian helpers
static inline uint16_t be16(const uint8_t *p){ return (uint16_t)((p[0]<<8)|p[1]); }
static inline uint32_t be32(const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3]; }
static inline void put_be16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static inline void put_be32(uint8_t *p, uint32_t v){
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v;
}

 
static bool read_rect(const uint8_t **p, uint32_t *len, CP_Rect *r){
    if (*len < 8) return false;
    const uint8_t *q = *p;
    r->x = be16(q); r->y = be16(q+2); r->w = be16(q+4); r->h = be16(q+6);
    *p += 8; *len -= 8; return true;
    }


static bool read_rect_array(const uint8_t **p, uint32_t *len, uint16_t *n, CP_Rect **out){
    if (*len < 2) return false;
    *n = be16(*p); *p += 2; *len -= 2;
    if (*n == 0) { *out = NULL; return true; }
    uint32_t need = (uint32_t)(*n) * 8u;
    if (*len < need) return false;
    CP_Rect *arr = (CP_Rect*)malloc((*n)*sizeof(CP_Rect));
    if (!arr) return false;
    for (uint16_t i=0;i<*n;i++){
        if (!read_rect(p, len, &arr[i])) { free(arr); return false; }
    }
    *out = arr; return true;
}


bool cp_recv_init_geom_sections(const uint8_t *p, uint32_t len){
    if (!read_rect(&p,&len,&g_geom.player)) return false;
    if (!read_rect_array(&p,&len,&g_geom.nPlat,   &g_geom.plat))   return false;
    if (!read_rect_array(&p,&len,&g_geom.nVines,  &g_geom.vines))  return false;
    if (!read_rect_array(&p,&len,&g_geom.nEnemies,&g_geom.enemies))return false;
    if (!read_rect_array(&p,&len,&g_geom.nFruits, &g_geom.fruits)) return false;
    return (len==0);
}




bool cp_read_header(int sock, struct CP_Header *h) {
    uint8_t raw[16];
    long r = net_read_n(sock, raw, sizeof(raw));
    if (r <= 0) return false;
    h->version    = raw[0];
    h->type       = raw[1];
    h->reserved   = be16(&raw[2]);
    h->clientId   = be32(&raw[4]);
    h->gameId     = be32(&raw[8]);
    h->payloadLen = be32(&raw[12]);
    return true;
}


bool cp_send_player_input(int sock, uint32_t clientId, uint32_t gameId,
                          uint8_t action, int16_t dx, int16_t dy)
{
    // payload = action(1) + dx(2) + dy(2) = 5 bytes
    uint8_t header[16], payload[5];
    header[0] = CP_VERSION;
    header[1] = CP_TYPE_PLAYER_INPUT;
    header[2] = 0; header[3] = 0;        // reserved
    put_be32(&header[4],  clientId);     // source (client)
    put_be32(&header[8],  gameId);
    put_be32(&header[12], 5);            // payloadLen

    payload[0] = action;
    put_be16(&payload[1], (uint16_t)dx);
    put_be16(&payload[3], (uint16_t)dy);

    if (net_write_n(sock, header, sizeof(header)) <= 0) return false;
    if (net_write_n(sock, payload, sizeof(payload)) <= 0) return false;
    return true;
}

/*void draw_from_matrix{
    matrix-->pintar
*/
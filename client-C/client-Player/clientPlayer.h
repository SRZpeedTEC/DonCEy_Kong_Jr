#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CP_VERSION 1
#define CP_TYPE_CLIENT_ACK    2
#define CP_TYPE_PLAYER_INPUT  3
#define CP_TYPE_INIT_GEOM     4

typedef struct { uint16_t x, y, w, h; } CP_Rect;

typedef struct {
    CP_Rect   player;
    uint16_t  nPlat, nVines, nEnemies, nFruits;
    CP_Rect  *plat, *vines, *enemies, *fruits;
} CP_Geom;

typedef struct CP_Header {                 // <<--- le damos tag
    uint8_t  version, type;
    uint16_t reserved;
    uint32_t clientId, gameId, payloadLen;
} CP_Header;

extern CP_Geom g_geom;

bool cp_read_header(int sock, CP_Header *h);            // firma única
bool cp_recv_init_geom_sections(const uint8_t *p, uint32_t len);
bool cp_send_player_input(int sock, uint32_t clientId, uint32_t gameId,
                          uint8_t action, int16_t dx, int16_t dy);

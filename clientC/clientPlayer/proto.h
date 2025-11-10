#pragma once
#include <stdint.h>
#include <stdbool.h>

#define CP_VERSION 1
#define CP_TYPE_CLIENT_ACK    2
#define CP_TYPE_PLAYER_INPUT  3
#define CP_TYPE_INIT_GEOM     4
#define CP_TYPE_INIT_STATIC   CP_TYPE_INIT_GEOM  // alias

typedef struct { uint16_t x,y,w,h; } CP_Rect;

typedef struct {
    uint16_t nPlat, nVines, nWater;
    CP_Rect *plat, *vines, *water;
} CP_Static;

typedef struct {
    uint8_t  version;
    uint8_t  type;
    uint16_t reserved;
    uint32_t clientId;
    uint32_t gameId;
    uint32_t payloadLen;
} CP_Header;

// protocolo
bool cp_read_header(int sock, CP_Header *h);
bool cp_recv_init_static_payload(const uint8_t *p, uint32_t len);

// acceso a los datos estáticos parseados
const CP_Static* cp_get_static(void);
void cp_free_static(void);

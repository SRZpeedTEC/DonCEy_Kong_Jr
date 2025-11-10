#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "raylib.h"

#include "../UtilsC/msg_types.h"
#include "../UtilsC/proto.h"
#include "../UI/Game/static_map.h"
#include "../UI/Game/game.h"
#include "net.h"

// ---- helper: ¿hay data lista para leer? (no bloquea) ----


// ---- TLV minimal ----
typedef struct { const uint8_t* p; uint32_t len; } TLVBuf;
static inline void tlv_init(TLVBuf* b, const uint8_t* p, uint32_t len){ b->p=p; b->len=len; }
static inline int tlv_next(TLVBuf* b, uint8_t* t, uint16_t* L, const uint8_t** V){
    if (b->len < 3) return 0;
    *t = b->p[0]; *L = (uint16_t)((b->p[1]<<8)|b->p[2]);
    if (b->len < (uint32_t)(3+*L)) return 0;
    *V = b->p+3; b->p += 3+*L; b->len -= 3+*L; return 1;
}

// ---- dispatcher ----
typedef void (*FrameHandler)(const uint8_t*, uint32_t);
static FrameHandler g_handlers[256];
static void disp_register(uint8_t type, FrameHandler fn){ g_handlers[type]=fn; }
static void disp_handle(uint8_t type, const uint8_t* p, uint32_t n){ if (g_handlers[type]) g_handlers[type](p,n); }

// ---- handlers ----
static void on_init_static(const uint8_t* p, uint32_t n){
    if (!cp_recv_init_static_payload(p,n)) fprintf(stderr,"[INIT_STATIC] payload invalido\n");
    else                                   fprintf(stdout,"[INIT_STATIC] OK\n");
}

static void on_state_bundle(const uint8_t* p, uint32_t n){
    TLVBuf b; tlv_init(&b,p,n);
    uint32_t tick=0;
    while (b.len){
        uint8_t t; uint16_t L; const uint8_t* V;
        if (!tlv_next(&b,&t,&L,&V)) break;

        if (t==TLV_STATE_HEADER && L>=4){
            tick = ((uint32_t)V[0]<<24)|((uint32_t)V[1]<<16)|((uint32_t)V[2]<<8)|V[3];
        } else if (t==TLV_PLAYER_CORR && L>=7){
            uint8_t grounded = V[0];
            int16_t platId   = (int16_t)((V[1]<<8)|V[2]);
            int16_t yCorr    = (int16_t)((V[3]<<8)|V[4]);
            int16_t vyCorr   = (int16_t)((V[5]<<8)|V[6]);
            game_apply_correction(tick, grounded, platId, yCorr, vyCorr);
        }
    }
}

// ---- envío de propuesta (cliente -> server) ----
static int send_player_proposed(int sock, uint32_t clientId, uint32_t tick,
                                int16_t x,int16_t y,int16_t vx,int16_t vy,uint8_t flags){
    uint8_t buf[4+2+2+2+2+1];
    buf[0]=tick>>24; buf[1]=tick>>16; buf[2]=tick>>8; buf[3]=tick;
    buf[4]=x>>8;  buf[5]=x;
    buf[6]=y>>8;  buf[7]=y;
    buf[8]=vx>>8; buf[9]=vx;
    buf[10]=vy>>8;buf[11]=vy;
    buf[12]=flags;
    return cp_send_frame(sock, CP_TYPE_PLAYER_PROP, clientId, 0, buf, sizeof buf) ? 1 : 0;
}

int main(int argc, char** argv){
    if (argc<3){ fprintf(stderr,"Usage: %s <ip> <port>\n", argv[0]); return 1; }

    if (!net_init()){ fprintf(stderr,"Net init fail\n"); return 1; }
    int sock = net_connect(argv[1], (uint16_t)atoi(argv[2]));
    if (sock < 0){ net_cleanup(); return 1; }

    // registrar handlers
    for (int i=0;i<256;i++) g_handlers[i]=NULL;
    disp_register(CP_TYPE_INIT_STATIC,  on_init_static);
    disp_register(CP_TYPE_STATE_BUNDLE, on_state_bundle);

    // ACK
    CP_Header h;
    if (!cp_read_header(sock,&h) || h.type!=CP_TYPE_CLIENT_ACK){
        fprintf(stderr,"Bad ACK\n"); goto done;
    }
    if (h.payloadLen){
        uint8_t* skip=(uint8_t*)malloc(h.payloadLen);
        if (skip){ net_read_n(sock,skip,h.payloadLen); free(skip); }
    }

    // INIT_STATIC
    if (!cp_read_header(sock,&h) || h.type!=CP_TYPE_INIT_STATIC){
        fprintf(stderr,"Expected INIT_STATIC\n"); goto done;
    } else {
        uint8_t* payload=(uint8_t*)malloc(h.payloadLen);
        if (!payload || net_read_n(sock,payload,h.payloadLen)<=0){ free(payload); goto done; }
        disp_handle(h.type, payload, h.payloadLen);
        free(payload);
    }

    // UI
    game_init(256,240,3);
    game_set_bg("clientC/UI/Sprites/FONDO1.png", 0.40f);

    uint32_t tick=0, clientId=0;

    while (!WindowShouldClose()){
        // lectura no bloqueante
        if (net_peek(sock)){
            if (!cp_read_header(sock,&h)) break;
            uint8_t* pl = (h.payloadLen)? (uint8_t*)malloc(h.payloadLen): NULL;
            if (h.payloadLen && net_read_n(sock,pl,h.payloadLen)<=0){ free(pl); break; }
            disp_handle(h.type, pl, h.payloadLen);
            free(pl);
        }

        ProposedState prop;
        game_update_and_get_proposal(cp_get_static(), &prop);
        send_player_proposed(sock, clientId, tick++, prop.x, prop.y, prop.vx, prop.vy, prop.flags);

        game_draw_static(cp_get_static());
    }

    game_shutdown();

done:
    cp_free_static();
    net_close(sock);
    net_cleanup();
    return 0;
}

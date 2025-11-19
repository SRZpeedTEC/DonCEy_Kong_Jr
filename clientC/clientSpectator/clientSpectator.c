#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "raylib.h"

#include "../UtilsC/msg_types.h"
#include "../UtilsC/proto.h"
#include "../UI/Game/static_map.h"
#include "../UI/Game/game.h"
#include "net.h"
#include "../UtilsC/tlv.h"

typedef void (*FrameHandler)(const uint8_t*, uint32_t);
static FrameHandler g_frameHandlers[256];


static void on_spectator_state(const uint8_t* payloadPtr, uint32_t payloadLen)
{
    // Esperamos: 2+2+2+2+1 = 9 bytes
    if (payloadLen < 9) {
        return;
    }

    int16_t x  = (int16_t)((payloadPtr[0] << 8) | payloadPtr[1]);
    int16_t y  = (int16_t)((payloadPtr[2] << 8) | payloadPtr[3]);
    int16_t vx = (int16_t)((payloadPtr[4] << 8) | payloadPtr[5]);
    int16_t vy = (int16_t)((payloadPtr[6] << 8) | payloadPtr[7]);
    uint8_t flags = payloadPtr[8];

    // Aplicamos el estado al jugador local del espectador
    game_apply_remote_state(x, y, vx, vy, flags);
}

// ---- spawn entities from the server ----
static void on_spawn_croc(const uint8_t *p, uint32_t n){
    uint8_t variant = 0; int16_t x=0, y=0;
    if (n == 4) { // legado
        x = (int16_t)((p[0]<<8)|p[1]); y = (int16_t)((p[2]<<8)|p[3]);
    } else if (n >= 5) {
        variant = p[0];
        x = (int16_t)((p[1]<<8)|p[2]); y = (int16_t)((p[3]<<8)|p[4]);
    } else return;

    game_spawn_croc(variant, x, y);
}


static void on_spawn_fruit(const uint8_t *p, uint32_t n){
    uint8_t variant = 0; int16_t x=0, y=0;
    if (n == 4) {
        x = (int16_t)((p[0]<<8)|p[1]); y = (int16_t)((p[2]<<8)|p[3]);
    } else if (n >= 5) {
        variant = p[0];
        x = (int16_t)((p[1]<<8)|p[2]); y = (int16_t)((p[3]<<8)|p[4]);
    } else return;

    game_spawn_fruit(variant, x, y);
}

static void on_remove_fruit(const uint8_t* p, uint32_t n){
    if (n < 4) return;
    int16_t x = (int16_t)((p[0]<<8) | p[1]);
    int16_t y = (int16_t)((p[2]<<8) | p[3]);
    game_remove_fruit_at(x, y);  // << delega al módulo de juego
}



typedef void (*FrameHandler)(const uint8_t*, uint32_t);
static FrameHandler g_frameHandlers[256];
static void disp_register(uint8_t frameType, FrameHandler handlerFn)
{ 
    g_frameHandlers[frameType] = handlerFn; 
}

static void disp_handle(uint8_t frameType, const uint8_t* payloadPtr, uint32_t payloadLen){ 
    if (g_frameHandlers[frameType]) 
    {
        g_frameHandlers[frameType](payloadPtr,payloadLen); 
    }
}



// ---- handlers ----
static void on_init_static(const uint8_t* payloadPtr, uint32_t payloadLen){
    if (!cp_recv_init_static_payload(payloadPtr,payloadLen)) fprintf(stderr,"[INIT_STATIC] payload invalido\n");
    else fprintf(stdout,"[INIT_STATIC] OK\n");
}


static void on_state_bundle(const uint8_t* payloadPtr, uint32_t payloadLen){
    TLVBuf tlvBuf; tlv_init(&tlvBuf,payloadPtr,payloadLen);
    uint32_t tick=0;
    while (tlvBuf.len){
        uint8_t tlvType; uint16_t tlvLen; const uint8_t* tlvValuePtr;
        if (!tlv_next(&tlvBuf,&tlvType,&tlvLen,&tlvValuePtr)) break;

        if (tlvType==TLV_STATE_HEADER && tlvLen>=4){
            tick = ((uint32_t)tlvValuePtr[0]<<24)|((uint32_t)tlvValuePtr[1]<<16)|((uint32_t)tlvValuePtr[2]<<8)|tlvValuePtr[3];
        } else if (tlvType==TLV_PLAYER_CORR && tlvLen>=7){
            uint8_t grounded = tlvValuePtr[0];
            int16_t platformId   = (int16_t)((tlvValuePtr[1]<<8)|tlvValuePtr[2]);
            int16_t yCorrection  = (int16_t)((tlvValuePtr[3]<<8)|tlvValuePtr[4]);
            int16_t vyCorrection = (int16_t)((tlvValuePtr[5]<<8)|tlvValuePtr[6]);
            game_apply_correction(tick, grounded, platformId, yCorrection, vyCorrection);
        }
    }
}



int main(int argc, char** argv) {

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    if (!net_init()) {
        fprintf(stderr, "net_init failed\n");
        return 1;
    }

    int socketFd = net_connect(argv[1], (uint16_t)atoi(argv[2]));
    if (socketFd < 0) {
        fprintf(stderr, "net_connect failed\n");
        net_cleanup();
        return 1;
    }

    // 1) Register handlers — same types as in clientPlayer.c
    for (int i = 0; i < 256; ++i) g_frameHandlers[i] = NULL;

    disp_register(CP_TYPE_INIT_STATIC,  on_init_static);
    disp_register(CP_TYPE_STATE_BUNDLE, on_state_bundle);
    disp_register(CP_TYPE_SPAWN_CROC,   on_spawn_croc);
    disp_register(CP_TYPE_SPAWN_FRUIT,  on_spawn_fruit);
    disp_register(CP_TYPE_REMOVE_FRUIT, on_remove_fruit);
    disp_register(CP_TYPE_SPECTATOR_STATE, on_spectator_state);

    CP_Header header;

    // 2) Read CLIENT_ACK (s -> c)
    if (!cp_read_header(socketFd, &header) || header.type != CP_TYPE_CLIENT_ACK) {
        fprintf(stderr, "Expected CLIENT_ACK\n");
        goto done;
    }
    if (header.payloadLen) {
        uint8_t* tmp = (uint8_t*)malloc(header.payloadLen);
        if (!tmp) goto done;
        if (net_read_n(socketFd, tmp, header.payloadLen) <= 0) {
            free(tmp);
            goto done;
        }
        free(tmp);
    }

    // 3) Read INIT_STATIC (static map, geometry, etc.)
    if (!cp_read_header(socketFd, &header) || header.type != CP_TYPE_INIT_STATIC) {
        fprintf(stderr, "Expected INIT_STATIC\n");
        goto done;
    } else {
        uint8_t* payloadBuf = (uint8_t*)malloc(header.payloadLen);
        if (!payloadBuf) goto done;

        if (net_read_n(socketFd, payloadBuf, header.payloadLen) <= 0) {
            free(payloadBuf);
            goto done;
        }
        // Let the existing handler process it (calls cp_recv_init_static_payload, etc.)
        disp_handle(header.type, payloadBuf, header.payloadLen);
        free(payloadBuf);
    }

    // 4) Initialize game/window (copy this from clientPlayer.c)
    // Use EXACTLY the same parameters you use in clientPlayer.c
    game_init(256, 240, 3);   // example; adjust if needed
    game_set_bg("clientC/UI/Sprites/FONDO1.png", 0.40f);

    // 5) Main spectator loop: ONLY receive and draw
    while (!WindowShouldClose()) {

        // If there is data, read and dispatch
        if (net_peek(socketFd) > 0) {
            if (!cp_read_header(socketFd, &header)) {
                fprintf(stderr, "cp_read_header failed\n");
                break;
            }

            uint8_t* payloadBuf = NULL;
            if (header.payloadLen) {
                payloadBuf = (uint8_t*)malloc(header.payloadLen);
                if (!payloadBuf) break;

                if (net_read_n(socketFd, payloadBuf, header.payloadLen) <= 0) {
                    free(payloadBuf);
                    break;
                }
            }

            disp_handle(header.type, payloadBuf, header.payloadLen);
            free(payloadBuf);
        }

        // IMPORTANT: no input, no send_player_proposed here.
        game_update_spectator(cp_get_static());
        game_draw_static(cp_get_static());
    }

    game_shutdown();

done:
    cp_free_static();
    net_close(socketFd);
    net_cleanup();
    return 0;
}

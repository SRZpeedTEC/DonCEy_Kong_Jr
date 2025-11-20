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


// ---- dispatcher ----
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

// --- handlers de órdenes del server ---
static void on_respawn_death(const uint8_t* p, uint32_t n){
    (void)p; (void)n;
    game_respawn_death();
}

static void on_respawn_win(const uint8_t* p, uint32_t n){
    (void)p; (void)n;
    game_respawn_win();
}

static void on_game_over(const uint8_t* p, uint32_t n){
    (void)p; (void)n;
    game_over_event();
}


// ---- envío de propuesta (cliente -> server) ----
static int send_player_proposed(int socketFd, uint32_t clientId, uint32_t tick, int16_t posX,int16_t posY,int16_t velX,int16_t velY,uint8_t flags)
{
    uint8_t outBuf[4+2+2+2+2+1];
    outBuf[0]=tick>>24; outBuf[1]=tick>>16; outBuf[2]=tick>>8; outBuf[3]=tick;
    outBuf[4]=posX>>8;  outBuf[5]=posX;
    outBuf[6]=posY>>8;  outBuf[7]=posY;
    outBuf[8]=velX>>8;  outBuf[9]=velX;
    outBuf[10]=velY>>8; outBuf[11]=velY;
    outBuf[12]=flags;
    return cp_send_frame(socketFd, CP_TYPE_PLAYER_PROP, clientId, 0, outBuf, sizeof outBuf) ? 1 : 0;
}

static int send_notify_death_collision(int socketFd, uint32_t clientId)
{
    return cp_send_frame(socketFd, CP_TYPE_NOTIFY_DEATH_COLLISION, clientId, 0, NULL, 0) ? 1 : 0;
}

static int send_notify_victory(int socketFd, uint32_t clientId)
{
    return cp_send_frame(socketFd, CP_TYPE_NOTIFY_VICTORY, clientId, 0, NULL, 0) ? 1 : 0;
}


// ---- main ----
int main(int argCount, char** argValues){

    // Parses args
    if (argCount<3){ fprintf(stderr,"Usage: %s <ip> <port>\n", argValues[0]); return 1; }

int run_player_client(const char* ip, uint16_t port) {

    // Network init + connect
    if (!net_init()) {
        fprintf(stderr, "Net init fail\n");
        return 1;
    }

    int socketFd = net_connect(ip, port); // socket TCP
    if (socketFd < 0) {
        net_cleanup();
        return 1;
    }

    // Register handlers
    for (int i=0;i<256;i++) g_frameHandlers[i]=NULL;
    disp_register(CP_TYPE_INIT_STATIC,  on_init_static);
    disp_register(CP_TYPE_STATE_BUNDLE, on_state_bundle);
    disp_register(CP_TYPE_SPAWN_CROC, on_spawn_croc);
    disp_register(CP_TYPE_SPAWN_FRUIT, on_spawn_fruit);
    disp_register(CP_TYPE_REMOVE_FRUIT, on_remove_fruit);
    disp_register(CP_TYPE_RESPAWN_DEATH_COLLISION, on_respawn_death);
    disp_register(CP_TYPE_RESPAWN_WIN, on_respawn_win);
    disp_register(CP_TYPE_GAME_OVER, on_game_over);



    // ACK
    CP_Header header;

    // Lee ACK
    if (!cp_read_header(socketFd,&header) || header.type!=CP_TYPE_CLIENT_ACK){
        fprintf(stderr,"Bad ACK\n"); goto done;
    }
    // Descarta payload si existe
    if (header.payloadLen){
        uint8_t* skipPayload=(uint8_t*)malloc(header.payloadLen);
        if (skipPayload){ net_read_n(socketFd,skipPayload,header.payloadLen); free(skipPayload); }
    }

    // INIT_STATIC
    // Verifica header
    if (!cp_read_header(socketFd,&header) || header.type!=CP_TYPE_INIT_STATIC){
        fprintf(stderr,"Expected INIT_STATIC\n"); goto done;
    } 

    // Lee payload
    else {
        uint8_t* payloadBuf=(uint8_t*)malloc(header.payloadLen);
        if (!payloadBuf || net_read_n(socketFd,payloadBuf,header.payloadLen)<=0){ free(payloadBuf); goto done; }
        disp_handle(header.type, payloadBuf, header.payloadLen);
        free(payloadBuf);
    }



    // UI
    game_init(256,240,3);
    game_set_bg("clientC/UI/Sprites/FONDO1.png", 0.40f);

    uint32_t tick=0, clientId=0;



    // Main loop Game
    while (!WindowShouldClose()){

        // Se revisa si hay datos del servidor

        if (net_peek(socketFd)){
            
            // Si no hay header, rompe
            if (!cp_read_header(socketFd,&header)) break;
            uint8_t* payloadBuf = (header.payloadLen)? (uint8_t*)malloc(header.payloadLen): NULL;
            
            
            if (header.payloadLen && net_read_n(socketFd,payloadBuf,header.payloadLen)<=0){ free(payloadBuf); break; }
            disp_handle(header.type, payloadBuf, header.payloadLen);
            free(payloadBuf);


        }

        ProposedState proposedState;
        game_update_and_get_proposal(cp_get_static(), &proposedState);

        if (game_consume_death_event()) {
            send_notify_death_collision(socketFd, clientId);
        }
        if (game_consume_win_event()) {
            send_notify_victory(socketFd, clientId);
        }

        send_player_proposed(socketFd, clientId, tick++, proposedState.x, proposedState.y, proposedState.vx, proposedState.vy, proposedState.flags);

        // debug: build entities TLV and show basic info (no network yet)
        uint8_t entitiesBuf[512];
        size_t entitiesLen = game_build_entities_tlv(entitiesBuf, sizeof(entitiesBuf));
        if (entitiesLen > 0) {
            //fprintf(stdout, "ENTITIES_TLV len = %zu\n", entitiesLen);
            (void)entitiesLen;
        }
        
        game_draw_static(cp_get_static());
    }

    game_shutdown();

done:
    cp_free_static();
    net_close(socketFd);
    net_cleanup();
    return 0;
}

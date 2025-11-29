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

// Dispatcher type for handling incoming frames
typedef void (*FrameHandler)(const uint8_t*, uint32_t);
static FrameHandler g_frameHandlers[256];

// ---- spectator state from the server ----
static void on_spectator_state(const uint8_t* payloadPtr, uint32_t payloadLen)
{
    // Expect: x,y,vx,vy (int16) + flags (uint8) = 9 bytes + entities TLV
    if (payloadLen < 9) {
        return;
    }

    // Parse player state (first 9 bytes)
    int16_t x  = (int16_t)((payloadPtr[0] << 8) | payloadPtr[1]);
    int16_t y  = (int16_t)((payloadPtr[2] << 8) | payloadPtr[3]);
    int16_t vx = (int16_t)((payloadPtr[4] << 8) | payloadPtr[5]);
    int16_t vy = (int16_t)((payloadPtr[6] << 8) | payloadPtr[7]);
    uint8_t flags = payloadPtr[8];

    // Apply the remote player state to the spectator view
    game_apply_remote_state(x, y, vx, vy, flags);

    // Parse entities TLV (bytes 9+)
    if (payloadLen > 9) {
        const uint8_t* tlvPtr = payloadPtr + 9;
        uint32_t tlvLen = payloadLen - 9;

        // Verify we have at least TLV header (type + length)
        if (tlvLen < 3) return;

        uint8_t tlvType = tlvPtr[0];
        uint16_t valueLen = (uint16_t)((tlvPtr[1] << 8) | tlvPtr[2]);

        // Verify it's the entities TLV
        if (tlvType != TLV_ENTITIES_CORR) return;

        // Verify we have the full value
        if (tlvLen < (uint32_t)(3 + valueLen)) return;

        const uint8_t* valuePtr = tlvPtr + 3;

        // First clear all existing entities (full snapshot)
        game_clear_all_entities();

        // Parse entity count
        if (valueLen < 1) return;
        uint8_t entityCount = valuePtr[0];
        const uint8_t* entityPtr = valuePtr + 1;

        // Parse each entity: kind(1) + spriteId(1) + x(2) + y(2) = 6 bytes
        for (uint8_t i = 0; i < entityCount; ++i) {
            if ((entityPtr - valuePtr) + 6 > valueLen) break;

            uint8_t kind = entityPtr[0];
            uint8_t spriteId = entityPtr[1];
            int16_t ex = (int16_t)((entityPtr[2] << 8) | entityPtr[3]);
            int16_t ey = (int16_t)((entityPtr[4] << 8) | entityPtr[5]);
            entityPtr += 6;

            // Skip player entity (kind 0) - position handled separately
            if (kind == 0) continue;

            // Spawn crocodile (kind 1)
            if (kind == 1) {
                uint8_t variant = (spriteId == 1) ? 2 : 1;
                game_spawn_croc(variant, ex, ey);
            }
            // Spawn fruit (kind 2)
            else if (kind == 2) {
                uint8_t variant;
                if (spriteId == 1) variant = 2;      // APPLE
                else if (spriteId == 2) variant = 3; // ORANGE
                else variant = 1;                     // BANANA (spriteId==3 or default)
                game_spawn_fruit(variant, ex, ey);
            }
        }
    }
}

// ---- gameplay event handlers ----
static void disp_register(uint8_t frameType, FrameHandler handlerFn)
{ 
    g_frameHandlers[frameType] = handlerFn; 
}

static void disp_handle(uint8_t frameType, const uint8_t* payloadPtr, uint32_t payloadLen)
{ 
    if (g_frameHandlers[frameType]) {
        g_frameHandlers[frameType](payloadPtr, payloadLen); 
    }
}

// ---- client → server: spectate request ----
static int send_spectate_request(int socketFd, uint8_t slot) {
    uint8_t buf[1];
    buf[0] = slot;  // 1 or 2 (target player slot)
    // clientId and gameId are set to 0; server already knows the real clientId
    return cp_send_frame(socketFd, CP_TYPE_SPECTATE_REQUEST, 0, 0, buf, sizeof(buf)) ? 1 : 0;
}

// ---- gameplay event handlers ----
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

static void on_game_restart(const uint8_t* p, uint32_t n){
    (void)p; (void)n;
    game_restart();
}

static void on_croc_speed_increase(const uint8_t* p, uint32_t n){
    (void)p; (void)n;
    crocodile_increase_speed();
}


// ---- INIT_STATIC handler ----
static void on_init_static(const uint8_t* payloadPtr, uint32_t payloadLen){
    if (!cp_recv_init_static_payload(payloadPtr, payloadLen)) {
        fprintf(stderr,"[INIT_STATIC] invalid payload\n");
    } else {
        fprintf(stdout,"[INIT_STATIC] OK\n");
    }
}

// ---- lives / score handlers (copied from player client) ----
static void on_lives_update(const uint8_t* p, uint32_t n){
    if (n >= 1) {
        game_set_ui_lives(p[0]);
    }
}

static void on_score_update(const uint8_t* p, uint32_t n){
    if (n >= 4){
        uint32_t sc = ((uint32_t)p[0] << 24) |
                      ((uint32_t)p[1] << 16) |
                      ((uint32_t)p[2] << 8)  |
                      (uint32_t)p[3];
        game_set_ui_score(sc);
    }
}

// ---- state bundle (TLV) handler ----
static void on_state_bundle(const uint8_t* payloadPtr, uint32_t payloadLen){
    TLVBuf tlvBuf; 
    tlv_init(&tlvBuf, payloadPtr, payloadLen);
    uint32_t tick = 0;

    while (tlvBuf.len) {
        uint8_t tlvType; 
        uint16_t tlvLen; 
        const uint8_t* tlvValuePtr;
        if (!tlv_next(&tlvBuf, &tlvType, &tlvLen, &tlvValuePtr)) break;

        if (tlvType == TLV_STATE_HEADER && tlvLen >= 4){
            tick = ((uint32_t)tlvValuePtr[0] << 24) |
                   ((uint32_t)tlvValuePtr[1] << 16) |
                   ((uint32_t)tlvValuePtr[2] << 8)  |
                   (uint32_t)tlvValuePtr[3];
        } else if (tlvType == TLV_PLAYER_CORR && tlvLen >= 7){
            uint8_t grounded      = tlvValuePtr[0];
            int16_t platformId    = (int16_t)((tlvValuePtr[1] << 8) | tlvValuePtr[2]);
            int16_t yCorrection   = (int16_t)((tlvValuePtr[3] << 8) | tlvValuePtr[4]);
            int16_t vyCorrection  = (int16_t)((tlvValuePtr[5] << 8) | tlvValuePtr[6]);

            game_apply_correction(tick, grounded, platformId, yCorrection, vyCorrection);
        }
    }
}

// ---- main spectator client entry point ----
int run_spectator_client(const char* ip, uint16_t port, uint8_t desiredSlot) {

    if (!net_init()) {
        fprintf(stderr, "net_init failed\n");
        return 1;
    }

    int socketFd = net_connect(ip, port);
    if (socketFd < 0) {
        fprintf(stderr, "net_connect failed\n");
        net_cleanup();
        return 1;
    }

    // Step 1: request SPECTATOR role
    {
        uint8_t requestedRole = 2;  // 2 = SPECTATOR
        if (net_write_n(socketFd, &requestedRole, 1) <= 0) {
            fprintf(stderr, "Failed to send requested role (SPECTATOR)\n");
            net_close(socketFd);
            net_cleanup();
            return 1;
        }
    }

    // Register frame handlers
    for (int i = 0; i < 256; ++i) g_frameHandlers[i] = NULL;
    disp_register(CP_TYPE_INIT_STATIC,             on_init_static);
    disp_register(CP_TYPE_STATE_BUNDLE,            on_state_bundle);
    // NOTE: SPAWN_CROC, SPAWN_FRUIT, REMOVE_FRUIT no longer used - entities come via TLV in SPECTATOR_STATE
    disp_register(CP_TYPE_SPECTATOR_STATE,         on_spectator_state);
    disp_register(CP_TYPE_RESPAWN_DEATH_COLLISION, on_respawn_death);
    disp_register(CP_TYPE_RESPAWN_WIN,             on_respawn_win);
    disp_register(CP_TYPE_GAME_OVER,               on_game_over);
    disp_register(CP_TYPE_LIVES_UPDATE,            on_lives_update);
    disp_register(CP_TYPE_SCORE_UPDATE,            on_score_update);
    disp_register(CP_TYPE_CROC_SPEED_INCREASE,     on_croc_speed_increase);
    disp_register(CP_TYPE_GAME_RESTART,            on_game_restart);

    CP_Header header;
    uint8_t roleByte = 0;

    // Step 2: read CLIENT_ACK (server → client)
    if (!cp_read_header(socketFd, &header) || header.type != CP_TYPE_CLIENT_ACK) {
        fprintf(stderr, "Expected CLIENT_ACK\n");
        goto done;
    }

    if (header.payloadLen > 0) {
        uint8_t* tmp = (uint8_t*)malloc(header.payloadLen);
        if (!tmp) goto done;

        if (net_read_n(socketFd, tmp, header.payloadLen) <= 0) {
            free(tmp);
            goto done;
        }

        roleByte = tmp[0];   // first byte = assigned role
        free(tmp);
    } else {
        fprintf(stderr, "CLIENT_ACK without payload (role byte missing)\n");
        goto done;
    }

    // This executable expects to be SPECTATOR (roleByte must be 2)
    if (roleByte != 2) {
        fprintf(stderr,
                "Server assigned role %u (expected SPECTATOR=2). Closing.\n",
                (unsigned)roleByte);
        goto done;
    }

    // Step 3: read INIT_STATIC (static map, geometry, etc.)
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

    // Step 3.5: tell the server which player slot we want to spectate (1 or 2)
    if (!send_spectate_request(socketFd, desiredSlot)) {
        fprintf(stderr, "Failed to send SPECTATE_REQUEST for slot %u\n",
                (unsigned)desiredSlot);
        goto done;
    }

    // Step 4: initialize the spectator game/window
    // Use the same resolution / scale as in clientPlayer.c
    game_init(256, 240, 3);   // adjust if you use different values
    game_set_bg("clientC/UI/Sprites/FONDO1.png", 0.40f);

    // Step 5: main spectator loop – only receive and draw (no input, no movement)
    while (!WindowShouldClose()) {

        // If there is data waiting, read and dispatch a frame
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

        // Local spectator update and drawing
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
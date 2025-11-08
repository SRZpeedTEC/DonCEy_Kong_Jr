// clientPlayer.c — portable client using net.*
// Build with net_posix.c on macOS/Linux/WSL, or net_win32.c on Windows.

#include "clientPlayer.h"
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// endian helpers
static inline uint16_t be16(const uint8_t *p){ return (uint16_t)((p[0]<<8)|p[1]); }
static inline uint32_t be32(const uint8_t *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|(uint32_t)p[3]; }
static inline void put_be16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static inline void put_be32(uint8_t *p, uint32_t v){
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v;
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

static void cp_free_rect_group(struct CP_RectGroup *group) {
    if (!group) return;
    free(group->rects);
    group->rects = NULL;
    group->rectCount = 0;
}

static const char *cp_element_type_name(uint8_t type) {
    switch (type) {
        case 1: return "PLAYER";
        case 2: return "ENEMY";
        case 3: return "FRUIT";
        case 4: return "PLATFORM";
        default: return "UNKNOWN";
    }
}

bool cp_recv_rect_payload(const uint8_t *payload, uint32_t payloadLen,
                          struct CP_RectState *outState) {
    if (!payload || !outState) return false;
    if (payloadLen < 2) return false;

    memset(outState, 0, sizeof(*outState));

    uint16_t groups = be16(&payload[0]);
    size_t offset = 2;

    struct CP_RectGroup *grp = NULL;
    if (groups > 0) {
        grp = (struct CP_RectGroup*)calloc(groups, sizeof(struct CP_RectGroup));
        if (!grp) return false;
    }

    for (uint16_t i = 0; i < groups; ++i) {
        if (offset + 3 > payloadLen) {
            for (uint16_t j = 0; j < i; ++j) cp_free_rect_group(&grp[j]);
            free(grp);
            return false;
        }
        grp[i].elementType = payload[offset++];
        uint16_t rectCount = be16(&payload[offset]);
        offset += 2;
        grp[i].rectCount = rectCount;

        size_t needed = (size_t)rectCount * 8;
        if (offset + needed > payloadLen) {
            for (uint16_t j = 0; j <= i; ++j) cp_free_rect_group(&grp[j]);
            free(grp);
            return false;
        }

        if (rectCount > 0) {
            grp[i].rects = (struct CP_Rect*)malloc(sizeof(struct CP_Rect) * rectCount);
            if (!grp[i].rects) {
                for (uint16_t j = 0; j < i; ++j) cp_free_rect_group(&grp[j]);
                free(grp);
                return false;
            }
        }

        for (uint16_t r = 0; r < rectCount; ++r) {
            grp[i].rects[r].x      = (int16_t)be16(&payload[offset]); offset += 2;
            grp[i].rects[r].y      = (int16_t)be16(&payload[offset]); offset += 2;
            grp[i].rects[r].width  = (int16_t)be16(&payload[offset]); offset += 2;
            grp[i].rects[r].height = (int16_t)be16(&payload[offset]); offset += 2;
        }
    }

    outState->groupCount = groups;
    outState->groups = grp;
    return true;
}

void cp_print_rect_state(const struct CP_RectState *state) {
    if (!state) return;
    printf("Rect State: %u groups\n", (unsigned)state->groupCount);
    for (uint16_t i = 0; i < state->groupCount; ++i) {
        const struct CP_RectGroup *grp = &state->groups[i];
        printf("  [%u] %s -> %u rects\n", (unsigned)i, cp_element_type_name(grp->elementType), (unsigned)grp->rectCount);
        for (uint16_t j = 0; j < grp->rectCount; ++j) {
            const struct CP_Rect *rect = &grp->rects[j];
            printf("      #%u : x=%d y=%d w=%d h=%d\n",
                   (unsigned)j, rect->x, rect->y, rect->width, rect->height);
        }
    }
}

void cp_free_rect_state(struct CP_RectState *state) {
    if (!state) return;
    for (uint16_t i = 0; i < state->groupCount; ++i) {
        cp_free_rect_group(&state->groups[i]);
    }
    free(state->groups);
    state->groups = NULL;
    state->groupCount = 0;
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

/*void draw_from_rect_state{
    rects-->pintar
*/
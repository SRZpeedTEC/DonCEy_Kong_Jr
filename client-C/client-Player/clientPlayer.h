#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h> // ssize_t if available

// ---- Protocol constants ----
#define CP_VERSION            1
#define CP_TYPE_RECT_STATE    1
#define CP_TYPE_CLIENT_ACK    2
#define CP_TYPE_PLAYER_INPUT  3



// ---- 16-byte header (big-endian for multi-byte fields) ----
struct CP_Header {
    uint8_t  version;
    uint8_t  type;
    uint16_t reserved;
    uint32_t clientId;
    uint32_t gameId;
    uint32_t payloadLen;
};

// Protocol helpers
bool cp_read_header(int sock, struct CP_Header *h);
struct CP_Rect {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
};

struct CP_RectGroup {
    uint8_t elementType;
    uint16_t rectCount;
    struct CP_Rect *rects;
};

struct CP_RectState {
    uint16_t groupCount;
    struct CP_RectGroup *groups;
};

bool cp_recv_rect_payload(const uint8_t *payload, uint32_t payloadLen,
                          struct CP_RectState *outState);
void cp_print_rect_state(const struct CP_RectState *state);
void cp_free_rect_state(struct CP_RectState *state);

// Send a small input event: action (e.g. 'L','R','U','D','J'), plus dx,dy (optional)
bool cp_send_player_input(int sock, uint32_t clientId, uint32_t gameId,
                          uint8_t action, int16_t dx, int16_t dy);

#endif // CLIENT_PLAYER_H

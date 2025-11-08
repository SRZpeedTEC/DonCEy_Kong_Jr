#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h> // ssize_t if available

// ---- Protocol constants ----
#define CP_VERSION            1
#define CP_TYPE_MATRIX_STATE  1
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
bool cp_recv_matrix_payload(const uint8_t *payload, uint32_t payloadLen,
                            uint16_t *outRows, uint16_t *outCols, uint8_t **outData);
void cp_print_matrix(uint16_t rows, uint16_t cols, const uint8_t *data);

// Send a small input event: action (e.g. 'L','R','U','D','J'), plus dx,dy (optional)
bool cp_send_player_input(int sock, uint32_t clientId, uint32_t gameId,
                          uint8_t action, int16_t dx, int16_t dy);

#endif // CLIENT_PLAYER_H

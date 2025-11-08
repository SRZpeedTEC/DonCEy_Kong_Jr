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

bool cp_recv_matrix_payload(const uint8_t *payload, uint32_t payloadLen,
                            uint16_t *outRows, uint16_t *outCols, uint8_t **outData) {
    if (payloadLen < 4) return false;
    uint16_t rows = be16(&payload[0]);
    uint16_t cols = be16(&payload[2]);
    uint32_t need = 4u + (uint32_t)rows * (uint32_t)cols;
    if (need != payloadLen) return false;

    uint8_t *data = (uint8_t*)malloc((size_t)rows * (size_t)cols);
    if (!data) return false;
    memcpy(data, payload + 4, (size_t)rows * (size_t)cols);

    *outRows = rows; *outCols = cols; *outData = data;
    return true;
}

void cp_print_matrix(uint16_t rows, uint16_t cols, const uint8_t *data) {
    printf("Matrix %u x %u\n", rows, cols);
    for (uint16_t r = 0; r < rows; ++r) {
        for (uint16_t c = 0; c < cols; ++c) {
            printf("%u ", (unsigned)data[r*cols + c]);
        }
        printf("\n");
    }
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
#ifndef CLIENT_PLAYER_H
#define CLIENT_PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h> // ssize_t

// ---- Protocol constants ----
#define CP_VERSION          1
#define CP_TYPE_MATRIX_STATE 1
#define CP_TYPE_CLIENT_ACK   2

// ---- 16-byte header (big-endian for multi-byte fields) ----
struct CP_Header {
    uint8_t  version;     // CP_VERSION
    uint8_t  type;        // CP_TYPE_*
    uint16_t reserved;    // 0 for now
    uint32_t clientId;    // sender on client->server, or destination on server->client
    uint32_t gameId;      // optional (0 if unused)
    uint32_t payloadLen;  // bytes after header
};

// ---- I/O helpers ----
ssize_t cp_read_n(int fd, void *buf, size_t n);
ssize_t cp_write_n(int fd, const void *buf, size_t n);

// ---- Net helpers ----
int  cp_connect_tcp(const char *ipv4, uint16_t port);

// ---- Protocol helpers ----
bool cp_read_header(int fd, struct CP_Header *h);

// Allocates *outData for rows*cols bytes. Caller must free(*outData).
bool cp_recv_matrix_payload(const uint8_t *payload, uint32_t payloadLen,
                            uint16_t *outRows, uint16_t *outCols, uint8_t **outData);

void cp_print_matrix(uint16_t rows, uint16_t cols, const uint8_t *data);

#endif // CLIENT_PLAYER_H

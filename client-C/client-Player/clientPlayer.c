// clientPlayer.c
// Builds on macOS/Linux (POSIX) for the sockets.

#include "clientPlayer.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------- I/O helpers ----------------

ssize_t cp_read_n(int fd, void *buf, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    size_t got = 0;
    while (got < n) {
        ssize_t r = read(fd, p + got, n - got);
        if (r == 0) return 0;                // peer closed
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        got += (size_t)r;
    }
    return (ssize_t)got;
}

ssize_t cp_write_n(int fd, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t sent = 0;
    while (sent < n) {
        ssize_t r = write(fd, p + sent, n - sent);
        if (r <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)r;
    }
    return (ssize_t)sent;
}

// ---------------- Net helpers ----------------

int cp_connect_tcp(const char *ipv4, uint16_t port) {
    int sock = -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ipv4, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", ipv4);
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

// ---------------- Protocol helpers ----------------

static inline uint16_t be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}
static inline uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

bool cp_read_header(int fd, struct CP_Header *h) {
    uint8_t raw[16];
    ssize_t r = cp_read_n(fd, raw, sizeof(raw));
    if (r <= 0) return false;

    h->version   = raw[0];
    h->type      = raw[1];
    h->reserved  = be16(&raw[2]);
    h->clientId  = be32(&raw[4]);
    h->gameId    = be32(&raw[8]);
    h->payloadLen= be32(&raw[12]);
    return true;
}

bool cp_recv_matrix_payload(const uint8_t *payload, uint32_t payloadLen,
                            uint16_t *outRows, uint16_t *outCols, uint8_t **outData) {
    if (payloadLen < 4) return false;
    uint16_t rows = be16(&payload[0]);
    uint16_t cols = be16(&payload[2]);
    uint32_t need = 4u + (uint32_t)rows * (uint32_t)cols;
    if (need != payloadLen) return false;

    uint8_t *data = (uint8_t *)malloc((size_t)rows * (size_t)cols);
    if (!data) return false;
    memcpy(data, payload + 4, (size_t)rows * (size_t)cols);

    *outRows = rows;
    *outCols = cols;
    *outData = data;
    return true;
}

void cp_print_matrix(uint16_t rows, uint16_t cols, const uint8_t *data) {
    printf("Matrix %u x %u\n", rows, cols);
    for (uint16_t r = 0; r < rows; ++r) {
        for (uint16_t c = 0; c < cols; ++c) {
            printf("%u ", (unsigned)data[r * cols + c]);
        }
        printf("\n");
    }
}

// ---------------- Main client ----------------

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    uint16_t port  = (uint16_t)atoi(argv[2]);

    int fd = cp_connect_tcp(ip, port);
    if (fd < 0) return 1;

    printf("Connected to %s:%u\n", ip, port);

    // Expect initial ACK (server sends it on connect)
    struct CP_Header h;
    if (!cp_read_header(fd, &h)) {
        fprintf(stderr, "Failed to read header\n");
        close(fd);
        return 1;
    }
    if (h.version != CP_VERSION) {
        fprintf(stderr, "Protocol version mismatch (got %u, expect %u)\n", h.version, CP_VERSION);
        close(fd);
        return 1;
    }
    if (h.type == CP_TYPE_CLIENT_ACK) {
        printf("Assigned clientId=%u\n", h.clientId);
        if (h.payloadLen > 0) {
            uint8_t *skip = (uint8_t *)malloc(h.payloadLen);
            if (!skip) { close(fd); return 1; }
            if (cp_read_n(fd, skip, h.payloadLen) <= 0) { free(skip); close(fd); return 1; }
            free(skip);
        }
    } else {
        // Not an ACK; swallow payload if present
        if (h.payloadLen > 0) {
            uint8_t *skip = (uint8_t *)malloc(h.payloadLen);
            if (!skip) { close(fd); return 1; }
            if (cp_read_n(fd, skip, h.payloadLen) <= 0) { free(skip); close(fd); return 1; }
            free(skip);
        }
    }

    // Main loop: receive messages
    while (true) {
        if (!cp_read_header(fd, &h)) {
            printf("Server closed.\n");
            break;
        }
        if (h.version != CP_VERSION) {
            fprintf(stderr, "Bad version: %u\n", h.version);
            break;
        }

        uint8_t *payload = NULL;
        if (h.payloadLen > 0) {
            payload = (uint8_t *)malloc(h.payloadLen);
            if (!payload) { fprintf(stderr, "OOM\n"); break; }
            if (cp_read_n(fd, payload, h.payloadLen) <= 0) {
                fprintf(stderr, "Failed reading payload\n");
                free(payload);
                break;
            }
        }

        if (h.type == CP_TYPE_MATRIX_STATE) {
            uint16_t rows=0, cols=0;
            uint8_t *data = NULL;
            if (!cp_recv_matrix_payload(payload, h.payloadLen, &rows, &cols, &data)) {
                fprintf(stderr, "Malformed MATRIX payload (len=%u)\n", h.payloadLen);
            } else {
                cp_print_matrix(rows, cols, data);
                free(data);
            }
        } else if (h.type == CP_TYPE_CLIENT_ACK) {
            printf("(ACK) clientId=%u gameId=%u\n", h.clientId, h.gameId);
        } else {
            printf("Unknown msg type=%u (len=%u)\n", h.type, h.payloadLen);
        }

        free(payload);
    }

    close(fd);
    return 0;
}

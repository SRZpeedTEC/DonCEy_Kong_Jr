// mainPlayer.c
// Client "main": connects, reads ACK, pulls initial RECT_STATE,
// then sends simple inputs (L/R/U/D/J) and prints updated rectangle lists.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>   // strchr
#include <ctype.h>    // toupper

#include "clientPlayer.h"
#include "net.h"

static int read_and_maybe_print_rect_state(int sock) {
    struct CP_Header h;
    if (!cp_read_header(sock, &h)) {
        printf("Server closed.\n");
        return -1;
    }

    uint8_t *payload = NULL;
    if (h.payloadLen > 0) {
        payload = (uint8_t*)malloc(h.payloadLen);
        if (!payload) { fprintf(stderr, "OOM\n"); return -1; }
        if (net_read_n(sock, payload, h.payloadLen) <= 0) {
            free(payload);
            return -1;
        }
    }

    if (h.type == CP_TYPE_RECT_STATE && payload) {
        struct CP_RectState state;
        if (cp_recv_rect_payload(payload, h.payloadLen, &state)) {
            cp_print_rect_state(&state);
            cp_free_rect_state(&state);
        }
    } else if (h.type == CP_TYPE_CLIENT_ACK) {
        // no-op; already handled earlier typically
    } else {
        // Unknown/other message types can be ignored or logged
        // printf("Got type=%u len=%u (ignored)\n", h.type, h.payloadLen);
    }

    free(payload);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    uint16_t port  = (uint16_t)atoi(argv[2]);

    if (!net_init()) {
        fprintf(stderr, "Network init failed\n");
        return 1;
    }

    int sock = net_connect(ip, port);
    if (sock < 0) { net_cleanup(); return 1; }

    printf("Connected to %s:%u\n", ip, port);

    // ---- Read initial ACK ----
    struct CP_Header h;
    if (!cp_read_header(sock, &h)) {
        fprintf(stderr, "Failed to read header\n");
        net_close(sock); net_cleanup();
        return 1;
    }
    if (h.version != CP_VERSION) {
        fprintf(stderr, "Protocol version mismatch (got %u, expect %u)\n", h.version, CP_VERSION);
        net_close(sock); net_cleanup();
        return 1;
    }
    uint32_t myId = 0, myGame = 0;
    if (h.type == CP_TYPE_CLIENT_ACK) {
        myId = h.clientId;
        myGame = h.gameId; // may be 0
        printf("Assigned clientId=%u\n", myId);
        // ACK has no payload in this protocol; if it ever does, swallow it:
        if (h.payloadLen > 0) {
            uint8_t *skip = (uint8_t*)malloc(h.payloadLen);
            if (!skip) { net_close(sock); net_cleanup(); return 1; }
            if (net_read_n(sock, skip, h.payloadLen) <= 0) { free(skip); net_close(sock); net_cleanup(); return 1; }
            free(skip);
        }
    } else {
        // If the first message wasn't an ACK, swallow any payload and continue
        if (h.payloadLen > 0) {
            uint8_t *skip = (uint8_t*)malloc(h.payloadLen);
            if (!skip) { net_close(sock); net_cleanup(); return 1; }
            if (net_read_n(sock, skip, h.payloadLen) <= 0) { free(skip); net_close(sock); net_cleanup(); return 1; }
            free(skip);
        }
    }

    // ---- Read and print the initial RECT_STATE (blocking, one message) ----
    // Some servers send it immediately after ACK; this consumes and prints it.
    if (read_and_maybe_print_rect_state(sock) < 0) {
        net_close(sock); net_cleanup();
        return 0;
    }

    // ---- Prompt loop: send inputs and read one response (rectangles) each time ----
    printf("> ");
    fflush(stdout);

    char line[64];
    while (fgets(line, sizeof(line), stdin)) {
        char action = 0;
        if (line[0] == 'q' || line[0] == 'Q') break;
        if (strchr("LRUDJlrudj", line[0])) action = (char)toupper((unsigned char)line[0]);

        if (action) {
            if (!cp_send_player_input(sock, myId, myGame, (uint8_t)action, 0, 0)) {
                fprintf(stderr, "Failed to send input\n");
                break;
            }
            // After sending an input, read the server's response (expected RECT_STATE)
            if (read_and_maybe_print_rect_state(sock) < 0) break;
        }

        printf("> ");
        fflush(stdout);
    }

    net_close(sock);
    net_cleanup();
    return 0;
}

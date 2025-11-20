#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "clientSpectator.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ip> <port> [slot]\n", argv[0]);
        fprintf(stderr, "  slot = 1 or 2 (optional, default = 1)\n");
        return 1;
    }

    const char* ip = argv[1];
    uint16_t port  = (uint16_t)atoi(argv[2]);

    uint8_t desiredSlot = 1;  // default: spectate player slot 1
    if (argc >= 4) {
        int s = atoi(argv[3]);
        if (s == 1 || s == 2) {
            desiredSlot = (uint8_t)s;
        } else {
            fprintf(stderr, "Invalid slot '%s', using default slot 1\n", argv[3]);
        }
    }

    return run_spectator_client(ip, port, desiredSlot);
}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "clientPlayer.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    const char* ip = argv[1];
    uint16_t port  = (uint16_t)atoi(argv[2]);

    return run_player_client(ip, port);
}
// net_win32.c
#include "net.h"
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

// We'll store SOCKET in an int by casting; that’s fine for our test client.
// For production, you can typedef a handle type.

bool net_init(void) {
    WSADATA w; return WSAStartup(MAKEWORD(2,2), &w) == 0;
}
void net_cleanup(void) { WSACleanup(); }

int net_connect(const char *ipv4, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { return -1; }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (InetPtonA(AF_INET, ipv4, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4: %s\n", ipv4);
        closesocket(s);
        return -1;
    }
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }
    return (int)s;
}

long net_read_n(int sock, void *buf, size_t n) {
    size_t got = 0; char *p = (char*)buf; SOCKET s = (SOCKET)sock;
    while (got < n) {
        int r = recv(s, p + got, (int)(n - got), 0);
        if (r == 0) return 0;
        if (r == SOCKET_ERROR) return -1;
        got += (size_t)r;
    }
    return (long)got;
}

long net_write_n(int sock, const void *buf, size_t n) {
    size_t sent = 0; const char *p = (const char*)buf; SOCKET s = (SOCKET)sock;
    while (sent < n) {
        int r = send(s, p + sent, (int)(n - sent), 0);
        if (r == SOCKET_ERROR) return -1;
        sent += (size_t)r;
    }
    return (long)sent;
}

void net_close(int sock) { closesocket((SOCKET)sock); }

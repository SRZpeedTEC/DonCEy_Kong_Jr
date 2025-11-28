// net_win32.c
#include "net.h"
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

// Initialize the WinSock2 networking system.
// Returns true if initialization succeeded.
bool net_init(void) {
    WSADATA w;
    return WSAStartup(MAKEWORD(2, 2), &w) == 0;
}

// Shut down WinSock2. Should be called once when the program closes.
void net_cleanup(void) {
    WSACleanup();
}

// Open a TCP connection to an IPv4 address and port.
// Returns: socket handle on success, or -1 on error.
int net_connect(const char *ipv4, uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    // Convert "192.168.0.10" → binary IPv4
    if (InetPtonA(AF_INET, ipv4, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4: %s\n", ipv4);
        closesocket(s);
        return -1;
    }

    // Attempt to connect
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }

    return (int)s; // success
}

// Read exactly 'n' bytes from a TCP socket into 'buf'.
// Returns:
//   >0 : number of bytes read
//    0 : connection closed by remote
//   -1 : read error
long net_read_n(int sock, void *buf, size_t n) {
    size_t got = 0;
    char *p = (char*)buf;
    SOCKET s = (SOCKET)sock;

    while (got < n) {
        int r = recv(s, p + got, (int)(n - got), 0);

        if (r == 0)        return 0;   // connection closed
        if (r == SOCKET_ERROR) return -1; // read error

        got += (size_t)r;
    }

    return (long)got;
}

// Write exactly 'n' bytes from 'buf' to a TCP socket.
// Returns:
//   >0 : number of bytes written
//   -1 : write error
long net_write_n(int sock, const void *buf, size_t n) {
    size_t sent = 0;
    const char *p = (const char*)buf;
    SOCKET s = (SOCKET)sock;

    while (sent < n) {
        int r = send(s, p + sent, (int)(n - sent), 0);

        if (r == SOCKET_ERROR)
            return -1; // send error

        sent += (size_t)r;
    }

    return (long)sent;
}

// Return how many bytes are available to read without blocking.
long net_peek(int sock) {
    u_long bytes = 0;
    ioctlsocket((SOCKET)sock, FIONREAD, &bytes);
    return (long)bytes;
}

// Close a socket (Windows version).
void net_close(int sock) {
    closesocket((SOCKET)sock);
}


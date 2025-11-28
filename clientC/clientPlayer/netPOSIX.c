#include "net.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

// Initializes the networking system (placeholder for platforms that need setup)
bool net_init(void) { 
    return true; 
}

// Cleans up networking state (placeholder for platforms that need teardown)
void net_cleanup(void) {}

// Open a TCP connection to an IPv4 address and port.
// Returns: socket fd on success, -1 on error.
int net_connect(const char *ipv4, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    // Convert string IPv4 ("127.0.0.1") into a network byte-order address
    if (inet_pton(AF_INET, ipv4, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", ipv4);
        close(s);
        return -1;
    }

    // Attempt to connect to the server
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }

    return s; // success
}

// Read exactly 'n' bytes from a socket into 'buf'.
// Returns: number of bytes read, 0 if connection closed, or -1 on error.
long net_read_n(int sock, void *buf, size_t n) {
    size_t got = 0;
    unsigned char *p = (unsigned char*)buf;

    while (got < n) {
        ssize_t r = read(sock, p + got, n - got);

        if (r == 0) return 0;               // connection closed
        if (r < 0) { 
            if (errno == EINTR) continue;   // interrupted → try again
            return -1;                      // read error
        }

        got += (size_t)r;
    }

    return (long)got;
}

// Write exactly 'n' bytes to the socket from 'buf'.
// Returns: number of bytes written, or -1 on error.
long net_write_n(int sock, const void *buf, size_t n) {
    size_t sent = 0;
    const unsigned char *p = (const unsigned char*)buf;

    while (sent < n) {
        ssize_t r = write(sock, p + sent, n - sent);

        if (r <= 0) {
            if (errno == EINTR) continue;   // interrupted → try again
            return -1;                      // write error
        }

        sent += (size_t)r;
    }

    return (long)sent;
}

// Returns how many bytes are available to read without blocking.
long net_peek(int sock) {
    int bytes = 0;
    ioctl(sock, FIONREAD, &bytes);
    return (long)bytes;
}

// Close a socket fd.
void net_close(int sock) { 
    close(sock); 
}


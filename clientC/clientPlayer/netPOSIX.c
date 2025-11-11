// net_posix.c
#include "net.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

bool net_init(void) { return true; }
void net_cleanup(void) {}

int net_connect(const char *ipv4, uint16_t port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ipv4, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IPv4: %s\n", ipv4);
        close(s);
        return -1;
    }
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }
    return s;
}

long net_read_n(int sock, void *buf, size_t n) {
    size_t got = 0; unsigned char *p = (unsigned char*)buf;
    while (got < n) {
        ssize_t r = read(sock, p + got, n - got);
        if (r == 0) return 0;
        if (r < 0) { if (errno == EINTR) continue; return -1; }
        got += (size_t)r;
    }
    return (long)got;
}

long net_write_n(int sock, const void *buf, size_t n) {
    size_t sent = 0; const unsigned char *p = (const unsigned char*)buf;
    while (sent < n) {
        ssize_t r = write(sock, p + sent, n - sent);
        if (r <= 0) { if (errno == EINTR) continue; return -1; }
        sent += (size_t)r;
    }
    return (long)sent;
}

#include <sys/ioctl.h>
long net_peek(int sock){
    int bytes = 0;
    ioctl(sock, FIONREAD, &bytes);
    return (long)bytes;
}


void net_close(int sock) { close(sock); }

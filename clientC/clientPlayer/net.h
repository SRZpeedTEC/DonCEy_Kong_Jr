#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


long net_peek(int sock);

// Cross-platform init/teardown (no-ops on POSIX)
bool net_init(void);
void net_cleanup(void);

// TCP connect, returns socket handle (int on POSIX, SOCKET cast to int on Win32 wrapper)
int  net_connect(const char *ipv4, uint16_t port);

// Exact-size I/O helpers (return <0 on error, 0 on peer close, n on success)
long net_read_n(int sock, void *buf, size_t n);
long net_write_n(int sock, const void *buf, size_t n);

void net_close(int sock);

#endif // NET_H

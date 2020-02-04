#ifndef FINN_H
#define FINN_H
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>

#define DUMP(data, len)                     \
    for (int i = 0; i < len; i++)           \
    {                                       \
        if (!(i%16)) putchar('\n');         \
        printf("0x%02hhx ", *(data+i));     \
    }                                       \
    putchar('\n')

#define FD_SET_MAX(max, fd, set)        \
    if (fd >= max) max = fd+1;          \
    FD_SET(fd, set)


#define MAX_PORT        5
#define MAX_USERID      255
#define MAX_PASSWD      255
#define MAX_ADDR_PORT   INET6_ADDRSTRLEN + MAX_PORT + 2
#define IOBUFF_SZ       8192

#include <util.h>
#include <config.h>
#include <vex_socket.h>
#include <socks.h>
#include <parse.h>

void usage(const char *name) __attribute__((noreturn));
struct proxy_config *init_proxy(int argc, char **argv);
void free_proxy_config(struct proxy_config *);
#endif
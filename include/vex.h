#ifndef VEX_H
#define VEX_H
#include <errno.h>
#include <ctype.h>
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

#include "v_util.h"
#include "v_config.h"
#include "v_socket.h"
#include "v_socks.h"
#include "v_parse.h"

void usage(const char *) __attribute__((noreturn));
struct proxy_config *init_proxy(int, char **);
void free_proxy_config(struct proxy_config *);
#endif

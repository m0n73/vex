#include <vex.h>

int read_a(int s, void *src, size_t len)
{
    size_t total = 0, left = len;
    ssize_t rlen = 0;

    while (total < len)
    {
        if ((rlen = read(s, src+total, left)) == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
            fprintf(stderr, "read: %s\n", strerror(errno));
            return -1;
        }
        left -= rlen;
        total += rlen;
    }
    return 0;
}

int write_a(int s, void *dst, size_t len)
{
    size_t total = 0, left = len;
    ssize_t wlen = 0;

    while (total < len)
    {
        if ((wlen = write(s, dst+total, left)) == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
            fprintf(stderr, "write: %s\n", strerror(errno));
            return -1;
        }
        left -= wlen;
        total += wlen;
    }
    return 0;
}

int toggle_sock_block(int sock, int blocking)
{
    int flags;

    if ((flags = fcntl(sock, F_GETFL, 0)) == -1)
    {
        fprintf(stderr, "fcntl: %s\n", strerror(errno));
        return -1;
    }

    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;

    if ((flags = fcntl(sock, F_SETFL, flags)) == -1)
    {
        fprintf(stderr, "fcntl: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

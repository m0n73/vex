#include <vex.h>

ssize_t read_a(int s, void *src, size_t *len)
{
    size_t total = 0, left = *len;
    ssize_t rlen = 0, offt;

    while (total < *len)
    {
        if ((rlen = read(s, src+total, left)) == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                *len = left;
                offt = (ssize_t) total;
                if (offt < 0) offt = -1;
                return offt;
            }

            LOGERR("read: %s\n", strerror(errno));
            return -1;
        }

        if (!rlen)
        {
            LOGERR("[-] Received EOF.\n");
            errno = EPIPE;
            return -1;
        }

        left -= rlen;
        total += rlen;
    }
    return 0;
}

ssize_t write_a(int s, void *dst, size_t *len)
{
    size_t total = 0, left = *len;
    ssize_t wlen = 0, offt ;

    while (total < *len)
    {
        if ((wlen = write(s, dst+total, left)) == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
            {
                *len = left;
                offt = (ssize_t) total;
                if (offt < 0) offt = -1;
                return offt;
            }
            LOGERR("write: %s\n", strerror(errno));
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
        LOGERR("fcntl: %s\n", strerror(errno));
        return -1;
    }

    if (blocking) flags &= ~O_NONBLOCK;
    else flags |= O_NONBLOCK;

    if ((flags = fcntl(sock, F_SETFL, flags)) == -1)
    {
        LOGERR("fcntl: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int timeout_wait(int s, long tmout, int test_set)
{
    int ret;
    fd_set conn_set;
    struct timeval tv;

    memset(&tv, '\0', sizeof(struct timeval));
    FD_ZERO(&conn_set);
    FD_SET(s, &conn_set);
    tv.tv_sec = tmout;

    switch (test_set)
    {
        case TM_READ:
            ret = select(s+1, &conn_set, NULL, NULL, &tv);
            break;
        case TM_WRITE:
            ret = select(s+1, NULL, &conn_set, NULL, &tv);
            break;
        case TM_ERROR:
            ret = select(s+1, NULL, NULL, &conn_set, &tv);
            break;
        default:
            LOGERR("[!] Invalid test set type\n");
            return -1;
    }

    if (ret == -1)
        LOGERR("select: %s\n", strerror(errno));

    return ret;
}

void strip_trail(char *str, size_t len)
{
    size_t i = 0;

    for (; i < len; i++)
    {
        if (!str[i]) return;
        if (isspace(str[i])) 
        {
            str[i] = '\0';
            return;
        }
    }
    return;
}

void *checked_calloc(size_t nmemb, size_t size)
{
    size_t test;

    test = nmemb * size;

    if (test)
        if (test/size == nmemb)
            return calloc(nmemb, size);

    errno = ERANGE;
    return NULL;
}

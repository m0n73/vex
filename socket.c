#include <vex.h>

int start_socket(const char *host, const char *port, int server)
{
    int sock_fd, gay_error, yes = 1, errno_s;
    struct addrinfo h, *r = NULL, *it = NULL;
    
    memset(&h, '\0', sizeof(struct addrinfo));
    h.ai_family = AF_UNSPEC;
    h.ai_socktype = SOCK_STREAM;
    h.ai_flags = server ? AI_PASSIVE : 0;

    if ((gay_error = getaddrinfo(host, port, &h, &r)))
    {
        fprintf(stderr, "getaddrinfo: %s.\n", gai_strerror(gay_error));
        errno = EINVAL;
        return -1;
    }
    
    it = r;

    do {
        if ((sock_fd = 
                socket(it->ai_family, it->ai_socktype, it->ai_protocol)) == -1)
            continue;

        if (server)
        {
            if (setsockopt(sock_fd, 
                SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
                continue;
            if (!bind(sock_fd, it->ai_addr, it->ai_addrlen)) break;
        } else {
            if (!connect(sock_fd, it->ai_addr, it->ai_addrlen)) break;
        }
        
        errno_s = errno;
        close(sock_fd);
        it = it->ai_next;
    } while (it);

    freeaddrinfo(r);

    if (!it) 
    {
        sock_fd = -1;
        errno = errno_s;
    }

    return sock_fd;
}

void event_loop(struct proxy_config *pc)
{
    int max = 0;
    ssize_t read_bytes = 0;
    char iobuff[IOBUFF_SZ];

    fd_set r_test, r_ready;

    FD_ZERO(&r_test);
    FD_ZERO(&r_ready);
    
    if (pc->bind_local)
    {
        if (listen(pc->listen_fd, 1) == -1)
        {
            fprintf(stderr, "listen: %s.\n", strerror(errno));
            return;
        }

        if ((pc->client_fd = accept(pc->listen_fd, NULL, NULL)) == -1)
        {
            fprintf(stderr, "accept: %s.\n", strerror(errno));
            return;
        }
    } else {
        pc->client_fd = pc->listen_fd;
    }

    FD_SET_MAX(max, pc->socks_fd, &r_test);
    FD_SET_MAX(max, pc->client_fd, &r_test);

    while (1)
    {
        memcpy(&r_ready, &r_test, sizeof(fd_set));

        if (select(max, &r_ready, NULL, NULL, NULL) == -1)
        {
            fprintf(stderr, "select: %s.\n", strerror(errno));
            return;
        }

        memset(iobuff, 0, IOBUFF_SZ);

        if (FD_ISSET(pc->client_fd, &r_ready))
        {
            if ((read_bytes = read(pc->client_fd, iobuff, IOBUFF_SZ)) == -1)
            {
                fprintf(stderr, "read: %s.\n", strerror(errno));
                return;
            }

            if (!read_bytes) 
            {
                printf("[-] EOF from the local listener.\n");
                return; 
            }

            if (write_a(pc->socks_fd, iobuff, read_bytes) == -1) return;
        }

        memset(iobuff, 0, IOBUFF_SZ);

        if (FD_ISSET(pc->socks_fd, &r_ready))
        {
            if ((read_bytes = read(pc->socks_fd, iobuff, IOBUFF_SZ)) == -1)
            {
                fprintf(stderr, "read: %s.\n", strerror(errno));
                return;
            }

            if (!read_bytes) 
            {
                printf("[-] EOF from the proxy.\n");
                return;
            }

            if (write_a(pc->client_fd, iobuff, read_bytes) == -1) return;
        }
    }
}

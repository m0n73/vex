#include <vex.h>

static int attempt_connect(int s, const struct sockaddr *address, 
        socklen_t addrlen, long tmout)
{
    int ret = 1, 
        err = 0;
    socklen_t len = sizeof(int);

    if (toggle_sock_block(s, 0) == -1) return -1;

    if (connect(s, address, addrlen) == -1)
    {
        ret = -1;
        if (errno == EINPROGRESS)
        {
            ret = timeout_wait(s, tmout, TM_WRITE);

            if (!ret)
                LOGUSR("[-] Connection timed out (%ld s)\n", tmout);
        } 

        if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
        {
            LOGERR("getsockopt: %s\n", strerror(err));
            return -1;
        }
    
        if (err)
        {
            LOGUSR("[-] Connection failed: %s\n", strerror(err));
            ret = -1;
        }

    }

    if (toggle_sock_block(s, 1) == -1) return -1;

    return ret;
}

int start_socket(const char *host, const char *port, int server, long tmout)
{
    int sock_fd, gay_error, yes = 1, errno_s;
    struct addrinfo h, *r = NULL, *it = NULL;
    
    memset(&h, '\0', sizeof(struct addrinfo));
    h.ai_family = AF_UNSPEC;
    h.ai_socktype = SOCK_STREAM;
    h.ai_flags = server ? AI_PASSIVE : 0;

    if ((gay_error = getaddrinfo(host, port, &h, &r)))
    {
        LOGERR("getaddrinfo: %s\n", gai_strerror(gay_error));
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
            if (attempt_connect(sock_fd, it->ai_addr, it->ai_addrlen, 
                        tmout) == 1) break;
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

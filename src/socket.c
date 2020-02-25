#include <vex.h>

int terminate = 0;

static void handle_termination(int sig)
{
    terminate = sig;
}

static struct sq_fifo *init_sq_fifo(void *buff)
{
    struct sq_fifo *ret = NULL;

    if (!(ret = (struct sq_fifo *) checked_calloc(1,
                    sizeof(struct sq_fifo))))
    {
        LOGERR("calloc failed: %s\n", strerror(errno));
        return NULL;
    }

    if (!buff)
    {
        if (!(ret->buff = (uint8_t *) 
                    checked_calloc(IOBUFF_SZ, sizeof(uint8_t))))
        {
            LOGERR("calloc failed: %s\n", strerror(errno));
            free(ret);
            ret = NULL;
        }
    } else {
        ret->buff = (uint8_t *) buff;
    }
    return ret;
}

static struct send_queue *init_sq(void)
{
    struct send_queue *ret = NULL;

    if (!(ret = (struct send_queue *) checked_calloc(1, 
                    sizeof(struct send_queue))))
    {
        LOGERR("calloc failed: %s\n", strerror(errno));
        return NULL;
    }

    ret->sq_len = 1;

    if (!(ret->sq_head = init_sq_fifo(NULL)))
    {
        free(ret);
        ret = NULL;
        return NULL;
    }

    ret->sq_tail = ret->sq_head;
    return ret;
}

static void free_sq_fifo(struct sq_fifo *head)
{
    struct sq_fifo *tmp = NULL,
                   *it = head;
    while (it)
    {
        if (it->buff)
        {
            free(it->buff);
            it->buff = NULL;
        }
        tmp = it->next;
        it->next = NULL;
        free(it);
        it = tmp;
    }
}

static void free_sq(struct send_queue *sq)
{
    if (sq)
    {
        free_sq_fifo(sq->sq_head);
        sq->sq_head = NULL;
        free(sq);
        sq = NULL;
    }
}

static void shift_sq(struct send_queue *sq)
{
    struct sq_fifo *tmp = NULL;

    if (sq)
    {
        if (!sq->sq_head->next)
        {
            memset(sq->sq_head->buff, '\0', IOBUFF_SZ);
            sq->sq_head->offt = 0;
            sq->sq_head->len = 0;
        } else {
            sq->sq_len--;
            if (sq->sq_head->buff)
            {
                free(sq->sq_head->buff);
                sq->sq_head->buff = NULL;
            }
            tmp = sq->sq_head->next;
            sq->sq_head->next = NULL;
            sq->sq_head = tmp;
        }
    }
}

static struct sq_fifo *add_sq(struct send_queue *sq, void *buff)
{
    if (!sq) 
    {
        errno = EINVAL;
        return NULL;
    }
    
    if (sq->sq_len == MAX_SEND_QUEUE)
    {
        LOGERR("[!] Max send queue reached\n");
        errno = ERANGE;
        return NULL;
    }

    sq->sq_len++;

    if (!(sq->sq_tail->next = init_sq_fifo(buff)))
        return NULL;

    sq->sq_tail = sq->sq_tail->next;

    return sq->sq_tail;
}

static ssize_t read_into_sq(int s, struct send_queue *sq)
{
    uint8_t *buff = NULL;
    ssize_t read_bytes = 0;

    if (!sq) 
    {
        errno = EINVAL;
        return -1;
    }

    if (!(buff = (uint8_t *) checked_calloc(IOBUFF_SZ, sizeof(uint8_t))))
    {
        LOGERR("calloc: %s\n", strerror(errno));
        return -1;
    }

    read_bytes = read(s, (void *) buff, IOBUFF_SZ*sizeof(uint8_t));

    if (read_bytes > 0)
    {
        if (!sq->sq_head->len)
        {
            if (sq->sq_head->buff)
                free(sq->sq_head->buff);
            sq->sq_head->buff = buff;
            sq->sq_head->len = read_bytes;
        } else {
            if (!add_sq(sq, buff))
            {
                LOGERR("add_sq: %s\n", strerror(errno));
                free(buff);
                buff = NULL;
                return -1;
            }
            sq->sq_tail->len = read_bytes;
        }
        return 1;
    } else {
        free(buff);
        buff = NULL;
        if (read_bytes == -1) 
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN) return 1;
            LOGERR("read: %s\n", strerror(errno));
        }
    }
    return read_bytes;
}

static int push_sq(int s, struct send_queue *sq)
{
    if (!sq)
    {
        errno = EINVAL;
        return -1;
    }

    struct sq_fifo *it_head = sq->sq_head;

    it_head->offt = write_a(s, it_head->buff + it_head->offt,
            (size_t *) &it_head->len);

    if (it_head->offt)
    {
        return it_head->offt;
    } else {
        if (!errno)
        {
            it_head = NULL;
            shift_sq(sq);
        }
        return 0;
    } 
}

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

void event_loop(struct proxy_config *pc)
{
    int max = 0, select_rd;
    struct send_queue *up_sq = init_sq(),
                      *dn_sq = init_sq();
    struct timeval tv;
    struct sigaction sigs;
    sigset_t temp_mask, old_mask;
    

    if (!up_sq || !dn_sq) return;

    fd_set r_test, r_ready, w_test, w_ready;

    memset(&sigs, '\0', sizeof(struct sigaction));
    memset(&tv, '\0', sizeof(struct timeval));

    tv.tv_sec = 1;

    sigs.sa_handler = handle_termination;
    sigs.sa_flags = SA_RESTART;
    sigemptyset(&sigs.sa_mask);
    sigemptyset(&temp_mask);

    sigaddset(&temp_mask, SIGINT);
    sigaddset(&temp_mask, SIGTERM);

    if (sigaction(SIGINT, &sigs, NULL) == -1)
    {
        LOGERR("sigaction: %s\n", strerror(errno));
        return;
    }

    if (sigaction(SIGTERM, &sigs, NULL) == -1)
    {
        LOGERR("sigaction: %s\n", strerror(errno));
        return;
    }

    sigprocmask(SIG_BLOCK, &temp_mask, &old_mask);

    FD_ZERO(&r_test);
    FD_ZERO(&r_ready);
    FD_ZERO(&w_test);
    FD_ZERO(&w_test);

    if (pc->bind_local)
    {
        if (listen(pc->listen_fd, 1) == -1)
        {
            LOGERR("listen: %s\n", strerror(errno));
            return;
        }

        if ((pc->client_fd = accept(pc->listen_fd, NULL, NULL)) == -1)
        {
            LOGERR("accept: %s\n", strerror(errno));
            return;
        }
    } else {
        pc->client_fd = pc->listen_fd;
    }

    toggle_sock_block(pc->socks_fd, 0);
    toggle_sock_block(pc->client_fd, 0);

    FD_SET_MAX(max, pc->socks_fd, &r_test);
    FD_SET_MAX(max, pc->client_fd, &r_test);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);

    while (1)
    {
        memcpy(&r_ready, &r_test, sizeof(fd_set));
        memcpy(&w_ready, &w_test, sizeof(fd_set));

        if ((select_rd = select(max, &r_ready, &w_ready, NULL, &tv)) == -1)
        {
            if (errno != EINTR)
            {
                LOGERR("select: %s\n", strerror(errno));
                goto exit_loop;
            } else {
                if (terminate) 
                {
                    sigprocmask(SIG_BLOCK, &temp_mask, &old_mask);
                    LOGUSR("[-] Terminating...\n");
                }
                continue;
            }
        }

        if (!select_rd) tv.tv_sec = 1;

        if (FD_ISSET(pc->client_fd, &w_ready))
        {
            switch (push_sq(pc->client_fd, dn_sq))
            {
                case -1:
                    LOGERR("push_sq: %s\n", strerror(errno));
                    goto exit_loop;
                default:
                    FD_SET(pc->client_fd, &w_test);
                    break;
                case 0:
                    if (!dn_sq->sq_head->len)
                        FD_CLR(pc->client_fd, &w_test);
                    else
                        FD_SET(pc->socks_fd, &w_test);
                    break;
            }
        }

        if (FD_ISSET(pc->socks_fd, &w_ready))
        {
            switch (push_sq(pc->socks_fd, up_sq))
            {
                case -1:
                    LOGERR("push_sq: %s\n", strerror(errno));
                    goto exit_loop;
                default:
                    FD_SET(pc->socks_fd, &w_test);
                    break;
                case 0:
                    if (!up_sq->sq_head->len)
                        FD_CLR(pc->socks_fd, &w_test);
                    else
                        FD_SET(pc->socks_fd, &w_test);
                    break;
            }
        }
        
        if (FD_ISSET(pc->client_fd, &r_ready))
        {
            if (read_into_sq(pc->client_fd, up_sq) <= 0) goto exit_loop;
            FD_SET(pc->socks_fd, &w_test);
        }

        if (FD_ISSET(pc->socks_fd, &r_ready))
        {
            if (read_into_sq(pc->socks_fd, dn_sq) <= 0) goto exit_loop;
            FD_SET(pc->client_fd, &w_test);
        }

        if (terminate) {
            goto exit_loop;
        }
    }
exit_loop:
    close(pc->client_fd);
    close(pc->socks_fd);
    free_sq(up_sq);
    up_sq = NULL;
    free_sq(dn_sq);
    dn_sq = NULL;
    return;
}

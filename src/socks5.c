#include <vex.h>

static int socks5_auth_userpass(struct proxy_config *pc)
{
    int tmout_state;
    uint8_t canvas[SOCKS5_USEPASSAUTH_BUFFER],
            ulen = (uint8_t) strnlen(pc->socks_conf->userid, MAX_USERID),
            plen = (uint8_t) strnlen(pc->socks_conf->passwd, MAX_PASSWD);
    size_t io_len;
    ssize_t offt = 0;

    memset(canvas, '\0', SOCKS5_USEPASSAUTH_BUFFER);

    *(canvas) = 0x01;
    *(canvas+1) = ulen;
    memcpy(canvas+2, pc->socks_conf->userid, ulen);
    *(canvas+2+ulen) = plen;
    memcpy(canvas+3+ulen, pc->socks_conf->passwd, plen);

    io_len = (3+ulen+plen)*(sizeof(uint8_t));

    while ((offt = write_a(pc->socks_fd, canvas+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_WRITE);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: write timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    io_len = 2*(sizeof(uint8_t));
    offt = 0;

    while ((offt = read_a(pc->socks_fd, canvas+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    if (canvas[1] == 0x00) 
    {
        LOGUSR("[+] SOCKS5 MSG: Username/Password authentication sucessful\n");
        return 0;
    } else {
        LOGERR("[-] SOCKS5 ERR: Username/Password authentication failed\n");
        return -1;
    }
}

static int socks5_negotiate_auth(struct proxy_config *pc)
{
    int i = 0;
    int tmout_state;
    uint8_t canvas[SOCKS5_NEGOTIATION_BUFFER]; 
    size_t io_len;
    ssize_t offt = 0;

    memset(canvas, '\0', SOCKS5_NEGOTIATION_BUFFER);
    *(canvas) = SOCKS5;
    *(canvas+1) = pc->socks_conf->no_methods;

    for (; i < pc->socks_conf->no_methods; i++)
        *(canvas+2+i) = pc->socks_conf->methods[i];

    io_len = (((size_t) pc->socks_conf->no_methods + 2)*(sizeof(uint8_t)));

    while ((offt = write_a(pc->socks_fd, canvas+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_WRITE);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: write timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    io_len = 2*(sizeof(uint8_t));
    offt = 0;

    while ((offt = read_a(pc->socks_fd, canvas+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    switch (*(canvas+1))
    {
        case NO_AUTH:
            return 0;
        case USERPASS:
            return socks5_auth_userpass(pc);
        case NO_METHODS:
            LOGERR("[-] SOCKS5 ERR: No acceptable methods\n");
            return -1;
        default:
            LOGERR("[-] SOCKS5 ERR: Method 0x%02hhx is not implemented\n",
                    *(canvas+1));
            return -1;
    }
}

static uint8_t socks5_recv_msg(struct proxy_config *pc)
{        
    int tmout_state;
    char addr_str[INET6_ADDRSTRLEN];
    struct socks5_msg msg;
    uint8_t addr[sizeof(struct in6_addr)];
    uint16_t port;
    size_t addr_size, io_len;
    ssize_t offt = 0;

    memset(&msg, '\0', sizeof(struct socks5_msg));
    memset(&addr, '\0', sizeof(struct in6_addr));
    memset(&addr_str, '\0', INET6_ADDRSTRLEN);

    io_len = sizeof(struct socks5_msg);

    while ((offt = read_a(pc->socks_fd, &msg+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return 0xff;

    switch (msg.atyp)
    {
        case IP_ADDRESS:
            addr_size = sizeof(struct in_addr);
            break;
        case IP6_ADDRESS:
            addr_size = sizeof(struct in6_addr);
            break;
        default:
            LOGERR("[-] SOCKS5 ERR: Proxy sent an unknown address type\n"); 
            return 0xff;
    }

    offt = 0;

    while ((offt = read_a(pc->socks_fd, addr+offt, &addr_size)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return 0xff;

    io_len = sizeof(uint16_t);
    offt = 0;

    while ((offt = read_a(pc->socks_fd, &port+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return 0xff;

    switch (msg.atyp)
    {
        case IP_ADDRESS:
            if (!inet_ntop(AF_INET, (void *) addr, addr_str, INET_ADDRSTRLEN))
            {
                LOGERR("inet_ntop: %s\n", strerror(errno));
                break;
            }
            break;
        case IP6_ADDRESS:
            if (!inet_ntop(AF_INET6, (void *) addr, addr_str, INET6_ADDRSTRLEN))
            {
                LOGERR("inet_ntop: %s\n", strerror(errno));
                break;
            }
            break;
        default:
            LOGERR("[-] SOCKS5 ERR: Proxy sent an unknown address type\n"); 
            return 0xff;
    }

    if (msg.code == SUCCESS)
    {
        LOGUSR("[+] SOCKS5 MSG: SUCCESS (%s:%d)\n", addr_str, ntohs(port));
        fflush(stdout);
    }

    return msg.code;
}

int socks5_attempt(struct proxy_config *pc)
{
    int tmout_state;
    uint8_t canvas[SOCKS5_IP6_REQ_BUFFER], reply_code;
    size_t addr_size, canvas_size;
    ssize_t offt = 0;

    memset(canvas, '\0', SOCKS5_IP6_REQ_BUFFER);

    *(canvas) = SOCKS5;
    *(canvas+1) = BIND;
    *(canvas+3) = pc->target->type;

    if (toggle_sock_block(pc->socks_fd, 0) == -1) return -1;

    if (socks5_negotiate_auth(pc) == -1)
        return -1;

    switch (*(canvas+3))
    {
        case IP_ADDRESS:
            addr_size = sizeof(struct in_addr);
            canvas_size = SOCKS5_IP4_REQ_BUFFER;
            break;
        case IP6_ADDRESS:
            addr_size = sizeof(struct in6_addr);
            canvas_size = SOCKS5_IP6_REQ_BUFFER;
            break;
        default:
            LOGERR("[-] SOCKS5 ERR: Unknown address type\n");
            return -1;
    }

    memcpy(canvas+4, pc->target->address, addr_size);
    memcpy(canvas+4+addr_size, &pc->target->port,
            sizeof(uint16_t));

    while ((offt = write_a(pc->socks_fd, canvas+offt, &canvas_size)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_WRITE);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS5 ERR: write timed out (%ld s)\n", 
                        pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    if ((reply_code = socks5_recv_msg(pc)) == SUCCESS)
    {
        if (toggle_sock_block(pc->socks_fd, 1) == -1) return -1;
        if ((reply_code = socks5_recv_msg(pc)) == SUCCESS)
            return 0;
    }

    switch (reply_code)
    {
        case SOCKS_FAIL:
            LOGERR("[-] SOCKS5 ERR: General SOCKS server failure\n");
            break;
        case NOT_ALLOWED:
            LOGERR("[-] SOCKS5 ERR: Connection not allowed\n");
            break;
        case NET_UNREACHABLE:
            LOGERR("[-] SOCKS5 ERR: Network unreachable\n");
            break;
        case HOST_UNREACHABLE:
            LOGERR("[-] SOCKS5 ERR: Target unreachable\n");
            break;
        case REFUSED:
            LOGERR("[-] SOCKS5 ERR: Connection refused\n");
            break;
        case TTL_EXPIRE:
            LOGERR("[-] SOCKS5 ERR: TTL expired\n");
            break;
        case CMD_NOT_SUPPORTED:
            LOGERR("[-] SOCKS5 ERR: Command not supported\n");
            break;
        case ADDR_NOT_SUPPORTED:
            LOGERR("[-] SOCKS5 ERR: Address type not supported\n");
            break;
        default:
            LOGERR("[-] SOCKS5 ERR: Unknown failure\n");
            break; 
    }
    return -1;
}

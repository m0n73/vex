#include <vex.h>

int socks4_attempt(struct proxy_config *pc)
{
    int tmout_state;
    char addr_str[INET_ADDRSTRLEN];
    uint8_t canvas[SOCKS4_REQ_BUFFER];
    size_t io_len, ulen = strnlen(pc->socks_conf->userid, MAX_USERID-1);
    ssize_t offt = 0;
    struct socks4_msg msg;

    memset(&msg, '\0', sizeof(struct socks4_msg));
    memset(canvas, '\0', SOCKS4_REQ_BUFFER);
    memset(addr_str, '\0', INET_ADDRSTRLEN);

    *(canvas) = SOCKS4;
    *(canvas+1) = BIND;
    memcpy(canvas+2, &pc->target->port, sizeof(uint16_t));
    memcpy(canvas+4, pc->target->address, sizeof(struct in_addr));
    memcpy(canvas+8, pc->socks_conf->userid, ulen*(sizeof(uint8_t)));

    io_len = (9+ulen)*(sizeof(uint8_t));

    if (toggle_sock_block(pc->socks_fd, 0) == -1) return -1;

    while ((offt = write_a(pc->socks_fd, canvas+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_WRITE);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS4 ERR: write timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    io_len = sizeof(struct socks4_msg);
    offt = 0;

    while ((offt = read_a(pc->socks_fd, &msg+offt, &io_len)) >= 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            tmout_state = timeout_wait(pc->socks_fd, pc->tmout, TM_READ);

            if (!tmout_state)
            {
                LOGUSR("[-] SOCKS4 ERR: read timed out (%ld s)\n", pc->tmout);
                return -1;
            } else if (tmout_state == -1) {
                return -1;
            }
        } else {
            break;
        }
    }

    if (offt == -1) return -1;

    if (toggle_sock_block(pc->socks_fd, 1) == -1) return -1;

    if (msg.code == GRANTED)
    {
        if (!inet_ntop(AF_INET, (void *) &msg.addr, 
                    addr_str, INET_ADDRSTRLEN))
        {
            LOGERR("inet_ntop: %s\n", strerror(errno));
            return -1;
        }

        LOGUSR("[+] SOCKS4 MSG: GRANTED (%s:%d)\n", 
                addr_str, ntohs(msg.port));

        io_len = sizeof(struct socks4_msg);

        if (read_a(pc->socks_fd, &msg, &io_len) == -1)
            return -1;

        if (msg.code == GRANTED)
        {
            if (!inet_ntop(AF_INET, (void *) &msg.addr,
                        addr_str, INET_ADDRSTRLEN))
            {
                LOGERR("inet_ntop: %s\n", strerror(errno));
                return -1;
            }

            LOGUSR("[+] SOCKS4 MSG: GRANTED (%s:%d)\n",
                addr_str, ntohs(msg.port));
            fflush(stdout);
            return 0;
        } 
    }

    switch (msg.code)
    {
        case REJECTED:
            LOGERR("[-] SOCKS4 ERR: Proxy rejected request\n");
            break;
        case UNREACHABLE:
            LOGERR("[-] SOCKS4 ERR: Proxy couldn't reach us\n");
            break;
        case IDENT_FAIL:
            LOGERR("[-] SOCKS4 ERR: Ident conversation failed\n");
            break;
        default:
            LOGERR("[-] SOCKS4 ERR: Unknown reply code\n");
            break;
    }
    return -1;
}

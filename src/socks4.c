#include <vex.h>

int socks4_attempt(struct proxy_config *pc)
{
    char addr_str[INET_ADDRSTRLEN];
    uint8_t canvas[SOCKS4_REQ_BUFFER];
    size_t ulen = strnlen(pc->socks_conf->userid, MAX_USERID-1);
    struct socks4_msg msg;

    memset(&msg, '\0', sizeof(struct socks4_msg));
    memset(canvas, '\0', SOCKS4_REQ_BUFFER);
    memset(addr_str, '\0', INET_ADDRSTRLEN);

    *(canvas) = SOCKS4;
    *(canvas+1) = BIND;
    memcpy(canvas+2, &pc->target->port, sizeof(uint16_t));
    memcpy(canvas+4, pc->target->address, sizeof(struct in_addr));
    memcpy(canvas+8, pc->socks_conf->userid, ulen*(sizeof(uint8_t)));

    if (write_a(pc->socks_fd, canvas, (9+ulen)*(sizeof(uint8_t))) == -1)
        return -1;

    if (read_a(pc->socks_fd, &msg, sizeof(struct socks4_msg)) == -1)
        return -1;

    if (msg.code == GRANTED)
    {
        if (!inet_ntop(AF_INET, (void *) &msg.addr, 
                    addr_str, INET_ADDRSTRLEN))
        {
            fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
            return -1;
        }

        printf("[+] SOCKS4 MSG: GRANTED (%s:%d)\n", 
                addr_str, ntohs(msg.port));

        if (read_a(pc->socks_fd, &msg, sizeof(struct socks4_msg)) == -1)
            return -1;

        if (msg.code == GRANTED)
        {
            if (!inet_ntop(AF_INET, (void *) &msg.addr,
                        addr_str, INET_ADDRSTRLEN))
            {
                fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
                return -1;
            }

            printf("[+] SOCKS4 MSG: GRANTED (%s:%d)\n",
                addr_str, ntohs(msg.port));
            fflush(stdout);
            return 0;
        } 
    }

    switch(msg.code)
    {
        case REJECTED:
            fprintf(stderr, "[-] SOCKS4 ERR: Proxy rejected request\n");
            break;
        case UNREACHABLE:
            fprintf(stderr, "[-] SOCKS4 ERR: Proxy couldn't reach us\n");
            fprintf(stderr, "[!] Look for a proxy that doesn't check ident\n");
            break;
        case IDENT_FAIL:
            fprintf(stderr, "[-] SOCKS4 ERR: Ident conversation failed\n");
            fprintf(stderr, "[!] Look for a proxy that doesn't check ident\n");
            break;
        default:
            fprintf(stderr, "[-] SOCKS4 ERR: Unknown reply code\n");
            break;
    }
    return -1;
}

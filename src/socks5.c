#include <vex.h>

static int socks5_auth_userpass(struct proxy_config *pc)
{
    uint8_t canvas[SOCKS5_USEPASSAUTH_BUFFER],
            ulen = (uint8_t) strnlen(pc->socks_conf->userid, MAX_USERID),
            plen = (uint8_t) strnlen(pc->socks_conf->passwd, MAX_PASSWD);

    memset(canvas, '\0', SOCKS5_USEPASSAUTH_BUFFER);

    *(canvas) = 0x01;
    *(canvas+1) = ulen;
    memcpy(canvas+2, pc->socks_conf->userid, ulen);
    *(canvas+2+ulen) = plen;
    memcpy(canvas+3+ulen, pc->socks_conf->passwd, plen);

    if (write_a(pc->socks_fd, canvas, 
                (3+ulen+plen)*(sizeof(uint8_t))) == -1) return -1;

    if (read_a(pc->socks_fd, canvas, 2*(sizeof(uint8_t))) == -1)
        return -1;
    
    if (canvas[1] == 0x00) 
    {
        printf("[+] Username/Password authentication sucessful\n");
        return 0;
    } else {
        fprintf(stderr, "[-] Username/Password authentication failed\n");
        return -1;
    }
}

static int socks5_negotiate_auth(struct proxy_config *pc)
{
    uint8_t canvas[SOCKS5_NEGOTIATION_BUFFER]; 

    memset(canvas, '\0', SOCKS5_NEGOTIATION_BUFFER);
    *(canvas) = SOCKS5;
    *(canvas+1) = pc->socks_conf->no_methods;
    *(canvas+2) = 0;

    if (write_a(pc->socks_fd, canvas, 
        (((size_t) pc->socks_conf->no_methods + 2)*(sizeof(uint8_t)))) == -1)
        return -1;

    if (read_a(pc->socks_fd, canvas, 2*(sizeof(uint8_t))) == -1)
        return -1;

    switch (*(canvas+1))
    {
        case NO_AUTH:
            return 0;
        case USERPASS:
            return socks5_auth_userpass(pc);
        case NO_METHODS:
            fprintf(stderr, "[-] No acceptable methods\n");
            return -1;
        default:
            fprintf(stderr, "[-] Method 0x%02hhx is not implemented\n",
                    *(canvas+1));
            return -1;
    }
}

static uint8_t socks5_recv_msg(struct proxy_config *pc)
{        
    char addr_str[INET6_ADDRSTRLEN];
    struct socks5_msg msg;
    uint8_t addr[sizeof(struct in6_addr)];
    uint16_t port;
    size_t addr_size;

    memset(&msg, '\0', sizeof(struct socks5_msg));
    memset(&addr, '\0', sizeof(struct in6_addr));
    memset(&addr_str, '\0', INET6_ADDRSTRLEN);

    if (read_a(pc->socks_fd, &msg, sizeof(struct socks5_msg)) == -1)
        return 0xff;

    switch(msg.atyp)
    {
        case IP_ADDRESS:
            addr_size = sizeof(struct in_addr);
            break;
        case IP6_ADDRESS:
            addr_size = sizeof(struct in6_addr);
            break;
        default:
            fprintf(stderr, 
                    "socks5_attempt: Proxy sent an unknown address type\n"); 
            return 0xff;
    }

    if (read_a(pc->socks_fd, addr, addr_size) == -1)
        return 0xff;

    if (read_a(pc->socks_fd, &port, sizeof(uint16_t)) == -1)
        return 0xff;

    switch(msg.atyp)
    {
        case IP_ADDRESS:
            if (!inet_ntop(AF_INET, (void *) addr, addr_str, INET_ADDRSTRLEN))
            {
                fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
                break;
            }
            break;
        case IP6_ADDRESS:
            if (!inet_ntop(AF_INET6, (void *) addr, addr_str, INET6_ADDRSTRLEN))
            {
                fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
                break;
            }
            break;
        default:
            fprintf(stderr, 
                    "socks5_attempt: Proxy sent an unknown address type\n"); 
            return 0xff;
    }

    if (msg.code == SUCCESS)
    {
        printf("[+] SOCKS5 MSG: SUCCESS (%s:%d)\n", addr_str, ntohs(port));
        fflush(stdout);
    }

    return msg.code;
}

int socks5_attempt(struct proxy_config *pc)
{
    uint8_t canvas[SOCKS5_IP6_REQ_BUFFER], reply_code;
    size_t addr_size, canvas_size;

    memset(canvas, '\0', SOCKS5_IP6_REQ_BUFFER);

    *(canvas) = SOCKS5;
    *(canvas+1) = BIND;
    *(canvas+3) = pc->target->type;

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
            fprintf(stderr, "socks5_attempt: Unknown address type\n");
            return -1;
    }

    memcpy(canvas+4, pc->target->address, addr_size);
    memcpy(canvas+4+addr_size, &pc->target->port,
            sizeof(uint16_t));

    if (write_a(pc->socks_fd, canvas, canvas_size) == -1) return -1;

    if ((reply_code = socks5_recv_msg(pc)) == SUCCESS)
        if ((reply_code = socks5_recv_msg(pc)) == SUCCESS)
            return 0;

    switch(reply_code)
    {
        case SOCKS_FAIL:
            fprintf(stderr, "[-] SOCKS5 ERR: General SOCKS server failure\n");
            break;
        case NOT_ALLOWED:
            fprintf(stderr, "[-] SOCKS5 ERR: Connection not allowed\n");
            break;
        case NET_UNREACHABLE:
            fprintf(stderr, "[-] SOCKS5 ERR: Network unreachable\n");
            break;
        case HOST_UNREACHABLE:
            fprintf(stderr, "[-] SOCKS5 ERR: Target unreachable\n");
            break;
        case REFUSED:
            fprintf(stderr, "[-] SOCKS5 ERR: Connection refused\n");
            break;
        case TTL_EXPIRE:
            fprintf(stderr, "[-] SOCKS5 ERR: TTL expired\n");
            break;
        case CMD_NOT_SUPPORTED:
            fprintf(stderr, "[-] SOCKS5 ERR: Command not supported\n");
            break;
        case ADDR_NOT_SUPPORTED:
            fprintf(stderr, "[-] SOCKS5 ERR: Address type not supported\n");
            break;
        default:
            fprintf(stderr, "[-] socks5_recv_msg failed\n");
            break; 
    }
    return -1;
}

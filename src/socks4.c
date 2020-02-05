#include <vex.h>

/* Request
 * VER | CMD | DSTPORT | DESTIP | ID
 *  1  |  1  |    2    |    4   | Var
 * -----------------------------------
 */

/* Reply
 *  VN | REP | DSTPORT | DESTIP 
 *  1  |  1  |    2    |    4   
 * -----------------------------
 */

int socks4_attempt(struct proxy_config *pc)
{
    uint8_t canvas[SOCKS4_REQ_BUFFER];
    size_t ulen = strnlen(pc->socks_conf->userid, MAX_USERID-1);
    struct socks4_msg msg;

    memset(&msg, '\0', sizeof(struct socks4_msg));
    memset(canvas, '\0', SOCKS4_REQ_BUFFER);

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
        printf("[*] SOCKS4 MSG: GRANTED (%s:%d)\n", inet_ntoa(msg.addr), 
                ntohs(msg.port));

        if (read_a(pc->socks_fd, &msg, sizeof(struct socks4_msg)) == -1)
            return -1;

        if (msg.code == GRANTED)
        {
            printf("[*] SOCKS4 MSG: GRANTED (%s:%d)\n",
                inet_ntoa(msg.addr), ntohs(msg.port));
            fflush(stdout);
            return 0;
        } 
    }

    switch(msg.code)
    {
        case REJECTED:
            fprintf(stderr, "[!] Proxy rejected request.\n");
            break;
        case UNREACHABLE:
            fprintf(stderr, "[!] Proxy couldn't reach us (is ident on?).\n");
            break;
        case IDENT_FAIL:
            fprintf(stderr, "[!] Ident conversation failed.\n");
            break;
        default:
            fprintf(stderr, "[!] Unknown reply code.\n");
            break;
    }
    return -1;
}

#include <vex.h>

static void *make_addr(uint8_t addr_type, const char * addr)
{
    int af;
    void *ret = NULL;
    uint8_t addr_buff[sizeof(struct in6_addr)];

    memset(&addr_buff, '\0', sizeof(struct in6_addr));

    switch(addr_type)
    {
        case IP_ADDRESS:
            af = AF_INET;
            break;
        case IP6_ADDRESS:
            af = AF_INET6;
            break;
        default:
            fprintf(stderr, "make_addr: Unsupported address\n");
            return NULL;
    }

    if (!inet_pton(af, (const char *) addr, (void *)addr_buff))
    {
        fprintf(stderr, "inet_pton: %s\n", strerror(errno));
        return NULL;
    }

    if (af == AF_INET)
    {
        if (!(ret  = calloc(1, sizeof(in_addr_t))))
        {
            fprintf(stderr, "calloc: %s\n", strerror(errno));
            return NULL;
        }
        memcpy(ret, &((struct in_addr *)addr_buff)->s_addr, sizeof(in_addr_t));
    } 

    if (af == AF_INET6)
    {
        if (!(ret = calloc(16, sizeof(unsigned char))))
        {
            fprintf(stderr, "calloc: %s\n", strerror(errno));
            return NULL;
        }
        memcpy(ret, ((struct in6_addr *)addr_buff)->s6_addr, 16);
    }

    return ret;
}

struct target_config *
init_target(uint8_t type, const char *addr, const char *port_str)
{
    uint32_t port;
    struct target_config *tc;

    if ((port = strtoul(port_str, NULL, 10)) == ULONG_MAX)
    {
        fprintf(stderr, "stroul: %s\n", strerror(errno));
        return NULL;
    }
    
    if (port > USHRT_MAX)
    {
        fprintf(stderr, "target_config: Invalid port\n");
        return NULL;
    }

    if (!(tc = 
            (struct target_config *) calloc(1, sizeof(struct target_config))))
    {
        fprintf(stderr, "calloc: %s\n", strerror(errno));
        return NULL;
    }

    tc->type = type;
    tc->port = htons((uint16_t)port);

    if (!(tc->address = make_addr(type, addr))) goto fail;
    return tc;

fail:
    if (tc)
    {
        if (tc->address)
        {
            free(tc->address);
            tc->address = NULL;
        }
        free(tc);
        tc = NULL;
    }
    return NULL;
}

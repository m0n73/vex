#include <vex.h>

struct socks_config *init_socks(uint8_t version, uint32_t methods)
{
    int i = 0;
    struct socks_config *sc;
    if (!(sc = (struct socks_config *) calloc(1, sizeof(struct socks_config))))
    {
        fprintf(stderr, "calloc: %s\n", strerror(errno));
        return NULL;
    }

    switch(sc->socks_version = version)
    {
        case SOCKS4:
            return sc;

        case SOCKS5:
            sc->no_methods = 1;
            sc->methods[i++] = 0;
            if (methods & USERPASS)
            {
                sc->no_methods++;
                sc->methods[i++] = USERPASS;
            }
            return sc;

        default:
            fprintf(stderr, "Invalid or unsupported SOCKS version\n");
            return NULL;
    }
    return NULL;
}

int attempt_socks_connect(struct proxy_config *pc)
{
    struct socks_list *it = pc->socks_list;
    while (it)
    {
        printf("[*] Trying the proxy at %s:%s\n", it->addr, it->port);
        if ((pc->socks_fd = start_socket(it->addr, it->port, 
                        0, pc->tmout)) != -1) 
        {
            printf("[+] Connected to %s:%s\n", it->addr, it->port);
            switch(pc->socks_conf->socks_version)
            {
                case SOCKS4:
                    if (socks4_attempt(pc) == -1) 
                    {
                        printf("[-] SOCKS4 negotiation failed\n");
                        close(pc->socks_fd);
                        goto next;
                    } 
                    return 0;
                case SOCKS5:
                    if (socks5_attempt(pc) == -1)
                    {
                        printf("[-] SOCKS5 negotiation failed\n");
                        close(pc->socks_fd);
                        goto next;
                    }
                    return 0;
                default:
                    fprintf(stderr, 
                            "[!] Invalid or unimplemented SOCKS version");
                    return -1;
            }
        }
        printf("[-] Connection to %s:%s failed\n", it->addr, it->port);
next:
        it = it->next;
    }
    fprintf(stderr, "[!] All SOCKS connections failed: Your list sucks!\n");
    return -1;
}

void free_socks_list(struct socks_list *head)
{
    struct socks_list *it = head, *temp;
    while (it)
    {
        if (it->addr)
        {
            free(it->addr);
            it->addr = NULL;
        }
        if (it->port)
        {
            free(it->port);
            it->port = NULL;
        }
        temp = it->next;
        it->next = NULL;
        free(it);
        it = temp;
    }
}

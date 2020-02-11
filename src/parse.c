#include <vex.h>

uint8_t parse_string_addr(const char *ip_port, char **ip, char **port)
{
    uint8_t addr_type = IP_ADDRESS;
    char *addr, *tmp;

    *ip = NULL;
    *port = NULL;
    
    if (*ip_port == '[') 
    {
        ip_port++;
        addr_type = IP6_ADDRESS;
    }

    if (!(addr = strndup(ip_port, MAX_ADDR_PORT)))
    {
        fprintf(stderr, "strndup: %s\n", strerror(errno));
        return 0;
    }

    tmp = strrchr(addr, ':');

    if (!tmp)
    {
        fprintf(stderr, "parse_string_addr: invalid address format\n");
        goto fail;
    }

    if (*(tmp-1) == ']') *(tmp-1) = '\0';

    *tmp++ = '\0';

    if (!(*port = strndup(tmp, MAX_PORT)))
    {
        fprintf(stderr, "strndup: %s\n", strerror(errno));
        goto fail;
    }
    
    if (!(*ip = strndup(addr, INET6_ADDRSTRLEN)))
    {
        fprintf(stderr, "strndup: %s\n", strerror(errno));
        goto fail;
    }

    strip_trail(*port, MAX_PORT);

    if (addr)
    {
        free(addr);
        addr = NULL;
    }
    return addr_type;

fail:
    if (addr)
    {
        free(addr);
        addr = NULL;
    }

    if (*port)
    {
        free(*port);
        *port = NULL;
    }

    if (*ip)
    {
        free(*ip);
        *ip = NULL;
    }
    return 0;
}

struct socks_list *parse_socks_list(const char *filepath)
{
    char *line_it = NULL,
         *port_it = NULL,
         *addr_it = NULL;
    size_t line_len = 0;
    struct socks_list *head=NULL, 
                      *it = NULL, 
                      *temp = NULL;
    FILE *socks_list = NULL;

    if (!(socks_list = fopen(filepath, "r")))
    {
        fprintf(stderr, "fopen: %s\n", strerror(errno));
        return NULL;
    }

    while ((line_len = getline(&line_it, &line_len, socks_list)) != -1)
    {
        if (line_len >= MAX_ADDR_PORT) goto cont;
        line_it[strcspn(line_it, "\r\n")] = '\0';
        if (!parse_string_addr(line_it, &addr_it, &port_it)) goto cont;
        if (!(temp  = (struct socks_list *) 
                    checked_calloc(1, sizeof(struct socks_list))))
        {
            fprintf(stderr, "calloc failed: %s\n", strerror(errno));
            free_socks_list(head);
            fclose(socks_list);
            return NULL;
        }

        if (!it) 
        {
            it = temp;
            head = it;
        } else { 
            it->next = temp; 
            it = it->next;
        }

        it->addr = addr_it;
        it->port = port_it;
cont:
        if (line_it)
        {
            free(line_it);
            line_it = NULL;
        }
        line_len = 0;
    }

    if (line_it)
    {
        free(line_it);
        line_it = NULL;
    }

    fclose(socks_list);
    return head;
}

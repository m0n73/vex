#ifndef CONFIG_H
#define CONFIG_H

#define MAX_METHODS         0xff

struct socks_list {
    char *addr;
    char *port;
    struct socks_list *next;
};

struct target_config {
    uint8_t type;
    void *address;
    uint16_t port;
};

struct socks_config {
    uint8_t socks_version;
    uint8_t no_methods;
    uint8_t methods[MAX_METHODS];
    const char * userid;
    const char * passwd;
};

struct proxy_config {
    char *loc_addr;  
    char *loc_port;  
    struct target_config *target;
    struct socks_list *socks_list;  
    struct socks_config *socks_conf;
    int socks_fd;
    int listen_fd;
    int client_fd;
    int bind_local;
};

#endif
#ifndef V_SOCKS_H
#define V_SOCKS_H

// SOCKS Version
#define SOCKS4              0x04
#define SOCKS5              0x05

// Authentication Methods
#define NO_AUTH             0x00
#define GSSAPI              0x01
#define USERPASS            0x02
#define NO_METHODS          0xff

//Commands
#define CONNECT             0x01
#define BIND                0x02
#define UDP_ASSOC           0x03

//Address types
#define IP_ADDRESS          0x01
#define DOMAINNAME          0x03
#define IP6_ADDRESS         0x04

// SOCKS4 Reply types
#define GRANTED             0x5a
#define REJECTED            0x5b
#define UNREACHABLE         0x5c
#define IDENT_FAIL          0x5d

// SOCKS5 Reply types
#define SUCCESS             0x00
#define SOCKS_FAIL          0x01
#define NOT_ALLOWED         0x02
#define NET_UNREACHABLE     0x03
#define HOST_UNREACHABLE    0x04
#define REFUSED             0x05
#define TTL_EXPIRE          0x06
#define CMD_NOT_SUPPORTED   0x07
#define ADDR_NOT_SUPPORTED  0x08

// SOCKS5 exchange buffer sizes
#define SOCKS5_USEPASSAUTH_BUFFER   MAX_USERID + MAX_PASSWD + 3
#define SOCKS5_NEGOTIATION_BUFFER   MAX_METHODS + 2
#define SOCKS5_REQ_BASE_BUFFER      sizeof(struct socks5_msg) + sizeof(uint16_t)
#define SOCKS5_IP4_REQ_BUFFER       \
    SOCKS5_REQ_BASE_BUFFER + sizeof(struct in_addr) 
#define SOCKS5_IP6_REQ_BUFFER       \
    SOCKS5_REQ_BASE_BUFFER + sizeof(struct in6_addr) 

#define SOCKS4_REQ_BUFFER                   \
    2 + sizeof(uint16_t) + sizeof(struct in_addr) + MAX_USERID

struct socks4_msg {
    uint8_t version;
    uint8_t code;
    uint16_t port;
    struct in_addr addr;
};

struct socks5_msg {
    uint8_t version;
    uint8_t code;
    uint8_t rsv;
    uint8_t atyp;
};

struct socks_config * init_socks(uint8_t, uint32_t);
struct target_config *init_target(uint8_t, const char *, const char *);
int attempt_socks_connect(struct proxy_config *);
void free_socks_list(struct socks_list *);

int socks4_attempt(struct proxy_config *);
int socks5_attempt(struct proxy_config *);
#endif

#ifndef V_SOCKET_H
#define V_SOCKET_H

#define MAX_SEND_QUEUE  10
#define IOBUFF_SZ       4096
#define SELECT_TMOUT    1

struct sq_fifo {
    uint8_t *buff;
    size_t offt;
    ssize_t len;
    struct sq_fifo *next;
};

struct send_queue {
    struct sq_fifo *sq_head;
    struct sq_fifo *sq_tail;
    size_t sq_len;
};

int start_socket(const char *, const char *, int, long);
void event_loop(struct proxy_config *);

#endif

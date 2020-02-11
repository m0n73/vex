#ifndef V_UTIL_H
#define V_UTIL_H

#define TM_READ     0
#define TM_WRITE    1
#define TM_ERROR    2

int write_a(int, void *, size_t);
int read_a(int, void *, size_t);
int toggle_sock_block(int, int);
int timeout_wait(int, long, int);
void strip_trail(char *, size_t);
void *checked_calloc(size_t, size_t);

#endif

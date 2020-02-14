#ifndef V_UTIL_H
#define V_UTIL_H

#define TM_READ     0
#define TM_WRITE    1
#define TM_ERROR    2

#define LOGERR(...)                \
    fprintf(stderr, __VA_ARGS__)

#define LOGUSR(...)                \
    printf(__VA_ARGS__)

#ifdef __VEX_DEBUG
#define LOGDBG(...)                \
    printf(__VA_ARGS__)
#else
#define LOGDBG(...) ;
#endif

ssize_t write_a(int, void *, size_t *);
ssize_t read_a(int, void *, size_t *);
int toggle_sock_block(int, int);
int timeout_wait(int, long, int);
void strip_trail(char *, size_t);
void *checked_calloc(size_t, size_t);

#endif

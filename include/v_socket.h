#ifndef V_SOCKET_H
#define V_SOCKET_H

int start_socket(const char *, const char *, int, long);
void event_loop(struct proxy_config *);

#endif

#ifndef SOCKET_DEFS_H
#define SOCKET_DEFS_H

int start_socket(const char *host, const char *port, int server);
void event_loop(struct proxy_config *);

#endif

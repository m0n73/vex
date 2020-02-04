CC=gcc

PROG=vex
INCLUDE=.
CFLAGS=-Wall -Werror -I $(INCLUDE)

SRC=socket.c main.c prox_init.c socks.c socks4.c socks5.c socks_target.c parse.c util.c
OBJ=$(SRC:.c=.o)

.PHONY: clean 

default: $(PROG)

$(PROG): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c $^ $(CFLAGS)

clean:
	-rm $(OBJ)
	-rm $(PROG)


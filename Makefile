CC=gcc

PROG=vex
INCLUDE=include/
CFLAGS=-Wall -Werror -I $(INCLUDE)

VPATH=src/
BUILD=build

SRC=socket.c main.c prox_init.c socks.c socks4.c socks5.c socks_target.c parse.c util.c
OBJ=$(SRC:%.c=$(BUILD)/%.o)

.PHONY: clean 

default: $(PROG)

$(PROG): $(OBJ)
	$(CC) -o $@ $^ 

$(OBJ): $(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@ 

clean:
	-rm $(OBJ)
	-rm $(PROG)


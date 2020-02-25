CC=gcc

PROG=vex
INCLUDE=include/
CFLAGS=-O2 -D_FORTIFY_SOURCE=2 -Werror=implicit-function-declaration -Wall -Werror -fpie -Wl,-pie -I $(INCLUDE)

VPATH=src/
BUILD=build

SRC=asyncio.c socket.c main.c prox_init.c socks.c socks4.c socks5.c socks_target.c parse.c util.c
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


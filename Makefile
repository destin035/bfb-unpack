CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

bfb-unpack: main.c
	$(CC) -o $@ $(CFLAGS) $<

clean:
	$(RM) bfb-unpack
.PHONY: clean

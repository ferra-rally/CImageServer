.PHONY: server clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra #-Werror
server:
	$(CROSS_COMPILE) server.c $(LIBS) $(CFLAGS) -o server
clean:
	rm -f server

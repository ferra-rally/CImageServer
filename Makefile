.PHONY: server server_conv clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra #-Werror
server:
	$(CROSS_COMPILE) -g *.c $(LIBS) $(CFLAGS) -o server
server_conv:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DIMAGE_CONVERTION -o server_conv
clean:
	rm -f server server_conv logFile.txt

.PHONY: server clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra -I/usr/include/python3.8 -lpython3.8 #-Werror
server:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -o server
server_conv:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DIMAGE_CONVERTION -o server_conv
clean:
	rm -f server server_conv logFile.txt

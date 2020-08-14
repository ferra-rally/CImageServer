.PHONY: server server_conv clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra -O3 #-Wno-stringop-overflow -Werror
server:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -o server
server_conv:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DIMAGE_CONVERTION -o server_conv
clean:
	rm -f server server_conv logFile.log
	rm -rf imagecache

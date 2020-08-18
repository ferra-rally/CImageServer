.PHONY: server server_conv clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra -O3 #-Wno-stringop-overflow -Werror
server:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -o server
server_log:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DLOG -o server
server_conv:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DIMAGE_CONVERTION -o server
server_conv_log:
	$(CROSS_COMPILE) *.c $(LIBS) $(CFLAGS) -DIMAGE_CONVERTION -DLOG -o server
clean:
	rm -f server logFile.log
	rm -rf imagecache

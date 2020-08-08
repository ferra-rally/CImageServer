.PHONY: server clean
CROSS_COMPILE ?= gcc
LIBS = -lpthread
CFLAGS = -Wall -Wextra -I/usr/include/python3.8 -lpython3.8 #-Werror
server:
	$(CROSS_COMPILE) -g *.c $(LIBS) $(CFLAGS) -o server
clean:
	rm -f server
	rm logFile.txt

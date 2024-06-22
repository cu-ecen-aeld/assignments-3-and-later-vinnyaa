# makefile for assignment 5 aesdsocket executable
CC=	gcc
OUTPUT= aesdsocket


# if cross compile needed, preppend to cc command
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)$(CC)
endif

aesdsocket: aesdsocket.c
	$(CC) aesdsocket.c -Wall -Werror -lpthread -lrt -o aesdsocket


# default build case
all: aesdsocket.c
	$(CC) aesdsocket.c -Wall -lpthread -Werror -lrt -o aesdsocket

clean:
	rm -rf *.o aesdsocket
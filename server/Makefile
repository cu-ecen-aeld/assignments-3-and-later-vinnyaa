# makefile for assignment 5 aesdsocket executable
CC ?=	gcc
TARGET ?= aesdsocket
TARGET_LDFLAGS ?= -lpthread -lrt


# if cross compile needed, preppend to cc command
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)$(CC)
endif

aesdsocket: aesdsocket.c
	$(CC) aesdsocket.c $(TARGET_LDFLAGS) -o $(TARGET)


# default build case
all: aesdsocket.c
	$(CC) aesdsocket.c $(TARGET_LDFLAGS) -o $(TARGET)

clean:
	rm -rf *.o $(TARGET)
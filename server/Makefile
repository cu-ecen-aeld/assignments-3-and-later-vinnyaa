# makefile for assignment 5 aesdsocket executable
CC?=	${CROSS_COMPILE}gcc
OUTPUT?= aesdsocket
LDFLAGS ?= -lpthread -lrt

default: all

all: $(OUTPUT)
# if cross compile needed, preppend to cc command
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)$(CC)
endif

$(OUTPUT): aesdsocket.c
	$(CC) aesdsocket.c $(LDFLAGS) -o aesdsocket


# default build case
all: aesdsocket.c
	$(CC) aesdsocket.c $(LDFLAGS) -o aesdsocket

clean:
	rm -rf *.o $(OUPUT)

# makefile for assignment 5 aesdsocket executable
CC ?= gcc
CFLAGS ?= -g Wall -Werror
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt


# if cross compile needed, preppend to cc command
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)$(CC)
endif

default:
	${CC} ${TARGET}.c -o ${TARGET} ${CFLAGS} ${LDFLAGS}
	
all:
	${CC} ${TARGET}.c -o ${TARGET} ${CFLAGS} ${LDFLAGS}

clean:
	rm -f ${TARGET} *.o
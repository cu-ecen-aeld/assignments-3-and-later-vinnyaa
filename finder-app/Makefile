# makefile for assignment 2 writer executable



# if cross compile needed, preppend to cc command
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)$(CC)
endif

writer: writer.c
	$(CC) writer.c -o writer


# default build case
all: writer.c
	$(CC) writer.c -o writer

clean:
	rm -rf *.o writer
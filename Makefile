CC = gcc

CFLAGS = -Wall --pedantic-errors

FILES = server.c client.c

EXES = $(FILES:.c=)

all: $(EXES)

clean:
	rm $(EXES)


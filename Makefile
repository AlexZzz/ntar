CC=gcc
CFLAGS=-std=c11 -D_BSD_SOURCE

OBJ=ntar.o

.PHONY: all clean install uninstall

all: ntar
clean:
	rm -f $(OBJ) ntar
ntar: ntar.o
	$(CC) $(OPT) -o ntar $(OBJ) $(CFLAGS)
install:
	install ./ntar /usr/local/bin
uninstall:
	rm -f /usr/local/bin/ntar


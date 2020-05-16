CC=gcc
CFLAGS=-g -O2 -pedantic -Wall -Werror

.PHONY: all
all: bin/echotest

bin/echotest: echotest.o msg.o error.o
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS)

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

.PHONY: doc
doc:
	mkdir -p man

.PHONY: clean
clean:
	rm -f *.o bin/echotest man/msg.3 man/draw.3

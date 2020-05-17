CC=gcc
CFLAGS=-g -O2 -pedantic -Wall -Werror

.PHONY: all
all: bin/blackjack

.PHONY: test
test: bin/echotest bin/drawtest

bin/blackjack: build/blackjack.o build/draw.o build/msg.o build/error.o
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS) -lncurses -ltinfo

bin/drawtest: build/drawtest.o build/draw.o build/error.o
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS) -lncurses -ltinfo

bin/echotest: build/echotest.o build/msg.o build/error.o
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS)

build/%.o: src/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

.PHONY: clean
clean:
	rm -rf bin build

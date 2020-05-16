CC=gcc
CFLAGS=-g -O2 -pedantic -Wall -Werror

.PHONY: all
all: bin/echotest

bin/echotest: build/echotest.o build/msg.o build/error.o
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS)

build/%.o: src/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

.PHONY: clean
clean:
	rm -f build/*.o bin/echotest
	rmdir build bin

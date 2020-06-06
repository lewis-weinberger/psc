# psc: Makefile
.POSIX:

include config.mk

all: lib blackjack

lib: lib/libpsc.so

blackjack: bin/blackjack

lib/libpsc.so: build/psc.o
	mkdir -p lib
	$(CC) -shared -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDLIBS)

bin/%: build/%.o lib/libpsc.so
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS) -Llib -lpsc

build/%.o: src/cmd/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

build/%.o: src/lib/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

clean:
	rm -rf bin lib build

install: lib/libpsc.o bin/blacjack man/man3/psc.3
	mkdir -p $(LIBPREFIX)
	cp -f lib/libpsc.so $(LIBPREFIX)/libpsc.so
	chmod 644 $(LIBPREFIX)/libpsc.so
	mkdir -p $(BINPREFIX)
	cp -f bin/blackjack $(BINPREFIX)/blackjack
	chmod 755 $(BINPREFIX)/blackjack
	mkdir -p $(MANPREFIX)/man3
	cp -f man/man3/psc.3 $(MANPREFIX)/man3/psc.3
	chmod 644 $(MANPREFIX)/man3/psc.3

uninstall:
	rm -f $(LIBPREFIX)/libpsc.so
	rm -f $(BINPREFIX)/blackjack
	rm -f $(MANPREFIX)/man3/psc.3

.PHONY: all lib blackjack clean install uninstall

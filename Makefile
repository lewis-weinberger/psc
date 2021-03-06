# psc: Makefile
.POSIX:

include config.mk

all: lib blackjack

lib: lib/libpsc.so

blackjack: bin/blackjack

lib/libpsc.so: build/psc.o
	mkdir -p lib
	$(CC) -shared -o $@ $^ $(CFLAGS) -lpthread

bin/%: build/%.o lib/libpsc.so
	mkdir -p bin
	$(CC) -o $@ $^ $(CFLAGS) -Llib -lpsc -Wl,-rpath,lib -L$(CURSESPATH) $(CURSESLIB)

build/%.o: src/cmd/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -I$(CURSESINC) -Iinclude/

build/%.o: src/lib/%.c
	mkdir -p build
	$(CC) -c -o $@ $< $(CFLAGS) -Iinclude/

clean:
	rm -rf bin lib build

install: lib/libpsc.so bin/blackjack man/man3/psc.3
	mkdir -p $(LIBPREFIX)
	cp -f lib/libpsc.so $(LIBPREFIX)/libpsc.so
	chmod 755 $(LIBPREFIX)/libpsc.so
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

# psc: config.mk
# Edit as appropriate

# Paths
PREFIX = /usr/local
LIBPREFIX = $(PREFIX)/lib
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

# Curses
CURSESINC = /usr/include
CURSESPATH = /usr/lib64
CURSESLIB = -lncurses -ltinfo

# Compiler flags
CC = gcc
CFLAGS = -I$(CURSESINC) -fPIC -O2 -pedantic -Wall -Werror

# Linker flags
LDFLAGS = -L$(CURSESPATH)
LDLIBS = -lpthread $(CURSESLIB)

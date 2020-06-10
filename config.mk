# psc: config.mk
# Edit as appropriate

# Paths
PREFIX = /usr/local
LIBPREFIX = $(PREFIX)/lib
BINPREFIX = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

# Curses
CURSESINC = /usr/include
CURSESPATH = /usr/lib
CURSESLIB = -lncurses -ltinfo

# Compiler flags
CC = gcc
CFLAGS = -fPIC -O2 -pedantic -Wall -Werror

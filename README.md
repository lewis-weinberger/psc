# psc

![](https://github.com/lewis-weinberger/psc/workflows/build/badge.svg)

> A *very* basic multiplayer game library.

This library was created for playing multiplayer games either locally on the same machine or remotely via, for example, an [ssh(1)](https://man.openbsd.org/ssh) connection. The primary use-case is for making terminal games with leisurely turn-based gameplay and text-based graphics in support of low bandwidth/latency internet connections.

[UNIX domain sockets](https://en.wikipedia.org/wiki/Unix_domain_socket) are used for communication between the different game instances (running on the same machine), using a client-server model. The library shares game state concurrently between a server instance and the client games, using [POSIX threads](https://en.wikipedia.org/wiki/POSIX_Threads). It works well in tandem with a TUI library such as [curses](https://en.wikipedia.org/wiki/Curses_(programming_library)), and indeed the example games use this for their interface design. Note that, at less than 800 SLOC, the library is very minimal &mdash; proper synchronisation of game state needs to be enforced by the user.

## Installation

The library and example games are written in C for POSIX.1-2008 compliant operating systems. Apart from a C compiler, the only other dependency (for the example games) is the [ncurses](https://invisible-island.net/ncurses/#downloads) library.

Edit [config.mk](./config.mk) to suit your system setup, then use the provided [Makefile](./Makefile) to build the library and games:

```sh
git clone https://github.com/lewis-weinberger/psc.git
cd psc
make
```

This will create directories `lib/` and `bin/` containing the library and games respectively. By default the games link to the library dynamically. To make just the library, use `make lib`. To make an individual game, use `make game` (substituting `game` for your desired game).

To install system-wide, use:

```sh
make install
```

By default this installs the (shared) library, games and manual page to `/usr/local` (see [config.mk](./config.mk)).

Any intermediate build artefacts that are not automagically removed by `make` will be stored in a `build/` directory. Use `make clean` to remove the compiled executables and object files. Similarly, `make uninstall` will remove installed files. Note that `make` needs to be run from the root directory of the repository.

## Documentation

Details of how to use the library can be found in [psc(3)](./man/man3/psc.3):

```sh
# assuming `make install` has put psc.3 somewhere on your MANPATH
man psc
# otherwise try
man ./man/man3/psc.3
```

Usage of the example games can be listed with the `-h` switch, for example:

```sh
blackjack -h
```

will print:

```
usage: blackjack [-h] [-n nclient]
-h:
        prints this help
-n:
        specify number of clients (server only)
```

## Example Games

The following games are being written for the collection:
- [x] [Blackjack](https://en.wikipedia.org/wiki/Blackjack): 2+ players

![blackjack start](./img/blackjack.png)
![blackjack finish](./img/blackjack2.png)

Other ideas for future games:
- [Labyrinth](https://en.wikipedia.org/wiki/Labyrinth_(paper-and-pencil_game)): 3+ players
- [Sternhalma](https://en.wikipedia.org/wiki/Chinese_checkers): 2, 3, 4 or 6 players
- [Go](https://en.wikipedia.org/wiki/Go_(game)): 2 players
- [Chess](https://en.wikipedia.org/wiki/Chess): 2 players

See also [this list of abstract strategy games](https://en.wikipedia.org/wiki/List_of_abstract_strategy_games).

## License

[MIT](./LICENSE)

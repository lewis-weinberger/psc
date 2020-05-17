# psc
> A collection of very simple multiplayer turn-based terminal games. 

This collection was created for playing multiplayer games either locally on the same machine or remotely via, for example, an [ssh(1)](https://man.openbsd.org/ssh) connection. The text-based graphics and leisurely turn-based gameplay were deliberately chosen to support poor internet connections.

[UNIX domain sockets](https://en.wikipedia.org/wiki/Unix_domain_socket) are used for communication between the different game instances (running on the same machine).

## Screenshots

![todo](./screenshot.png)

## Installation

These games are written in C for POSIX-compliant operating systems. Apart from a C compiler, the only other dependency is the [ncurses](https://invisible-island.net/ncurses/#downloads) library. The provided [Makefile](./Makefile) will build all the games in a `bin/` directory in the working directory:

```sh
git clone https://github.com/lewis-weinberger/psc.git
cd psc
make
```

To make an individual game, use `make name-of-game` (substituting `name-of-game` for your desired game).

Build artefacts (compiled object files) will be stored in a `build/` directory. Use `make clean` to remove the compiled executables and object files.

## Usage

Starting each game follows the same pattern. Below we will use `game` to generically refer to any of the game exectuables (substitute your desired game as appropriate).

To start the host game with `N` other players using socket `/tmp/game_socket`:
```sh
game -h N /tmp/game_socket
```
You can confirm creation of the socket by examining your filesystem (in this example, `ls /tmp` would show the created socket file). The remaining players can then join this game:
```sh
game /tmp/socket
```

Each game is turn-based, and will indicate when it is the given player's turn. 

Note that if a client game terminates then the host game will wait until a new player joins to fill their place. If the host game terminates then all the clients will also terminate. When the game is finished the socket will be unlinked.

## Writing new games

The collection has been designed in a modular way such that the connections and terminal drawing components can be reused. In particular the game uses very simple (blocking) stream communication between the clients and the server, designed for turn-based gameplay. Alongside this it employs the ncurses library to display the text-based graphics on the terminal.

Documentation describing this functionality is available in the msg(3) and draw(3) manpages. Assuming you have `man` installed, these can be read with:

```sh
man msg.3
man draw.3
```

Following this documentation and using existing game examples, it should be straightforward to write new games.

## License

[MIT](./LICENSE)

---
name: sockets-build
description: Use when building, installing, cleaning, cross-compiling for RPi, or running the sockets TCP file-transfer project. Trigger keywords: "make", "compile", "build sockets", "sockets project", "cross-compile", "raspberry pi".
---

# Sockets Build (C++17 TCP File Transfer)

## Build

```bash
make all          # build master + slave
make master       # build only master
make slave        # build only slave
```

## Cross-compile for Raspberry Pi

```bash
make CROSS_COMPILE=arm-linux-gnueabihf- all
```

## Install

```bash
sudo make install                # install to /usr/local/bin
make install PREFIX=/opt         # custom prefix
make uninstall                   # uninstall
```

## Clean

```bash
make clean        # remove bin/ and obj/
```

## Run

**Server (Raspberry Pi):**
```bash
./bin/slave [opciones] <port>
  -o <dir>   output directory (default: .)
  -d         daemon mode (background)

./bin/slave 5000
./bin/slave -o ~/recibidos 5000
```

**Client (PC):**
```bash
./bin/master [opciones] <file> <IP> <port>
  -q         quiet mode (no progress)
  -t <ms>    timeout in ms (default: 5000)

./bin/master doc.pdf 192.168.1.100 5000
```

## Files

- `include/protocol.h` — protocol constants (BUFFER_SIZE, ACK, etc.)
- `include/socket_utils.h` — RAII Socket wrapper, sendAll/recvAll, makeServerSocket
- `src/master.cpp` — TCP client with timeout, partial-send handling, verbose flag
- `src/slave.cpp` — TCP server with SO_REUSEADDR, fork multi-connection, SIGCHLD, output dir
- `Makefile` — build system with install/uninstall/dist targets and cross-compile support

## Architecture

- POSIX sockets (no external dependencies)
- Server forks a child per connection
- ACK-per-packet protocol for reliable transfer
- Compatible with Ubuntu x86_64 and Raspberry Pi OS ARM

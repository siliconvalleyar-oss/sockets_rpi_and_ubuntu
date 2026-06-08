---
name: sockets-build
description: Use when building, cleaning, or running the sockets C++ project with Makefile. Trigger keywords: "make", "compile", "build sockets", "sockets project".
---

# Sockets Build (C++)

## Build

```bash
make all          # build master + slave
make master       # build only master
make slave        # build only slave
```

## Clean

```bash
make clean        # remove binaries and object files
```

## Run

```bash
# Slave (Raspberry Pi / server)
./bin/slave <port>

# Master (PC / client)
./bin/master <file> <IP> <port>
```

## Files

- `src/master.cpp` — client that sends a file
- `src/slave.cpp` — server that receives a file
- `Makefile` — build system (C++17, g++)

## Requirements

- g++ with C++17 support
- Linux (sockets API via `<sys/socket.h>`)

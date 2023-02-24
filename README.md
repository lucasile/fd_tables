# FD Table Tool for Linux. Made for CSCB09

Author: Lucas Ilea

# Table of contents

<!--ts-->

- [Documentation](#documentation)
_ [Compilation](#compilation)
_ [Arguments](#arguments) \* [Code](#code)
<!--te-->

## Documentation

## Compilation

To compile to an executable, run

`$ make`

with the `Makefile` in the same directory.

## Arguments

Make sure you run the program with the `structprocess.h` header file in the same directory.

```bash
./fd_tables <pid> (pid is a positional argument that must be first)
./fd_tables --per-process (print the pid and fd table)
./fd_tables --systemWide (print the pid, fd, and symlink table)
./fd_tables --composite (print the pid, fd, inode, and symlink table)
./fd_tables --Vnodes (print the fd and inode table)
./fd_tables --threshold=X (set a threshold to flag processes with fd > X. Note: this will filter all processes irregardless if a pid was specified as a positional argument)
```

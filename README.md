# FD Table Tool for Linux. Made for CSCB09

Author: Lucas Ilea

# Table of contents

<!--ts-->

- [Documentation](#documentation) \* [Compilation](#compilation)
  - [Arguments](#arguments)
  - [Code](#code)

<!--te-->

## Documentation

## Compilation

To compile to an executable, run

`$ make`

while in the directory of the repository.

## Arguments

Make sure you run the program with the `structprocess.h` header file in the same directory.

```bash
./fd_tables <pid> (pid is a positional argument that must be first)
./fd_tables --per-process (print the pid and fd table)
./fd_tables --systemWide (print the pid, fd, and symlink table)
./fd_tables --composite (print the pid, fd, inode, and symlink table)
./fd_tables --Vnodes (print the fd and inode table)
./fd_tables --threshold=X (set a threshold to flag processes with fd > X. Note: this will filter all processes irregardless if a pid was specified as a positional argument)
./fd_tables --output_binary (using file mode 'wb' create/write to file called compositeTable.bin with the composite table in binary format)
./fd_tables --output_TXT (using file mode 'w' create/write to file called compositeTable.txt with the composite table in text (ASCII))
```

By default running `$ ./fd_tables` will run the program with the composite flag.
Specifying a PID as the positional argument, `--output_binary`, or `--output_TXT`, will
still run the program in composite mode, unless otherwise specified.

---

## Analysis

Using these two commands, I was able to append to some files to track the time it took to run
the program for each output type, and the corresponding file size.

`$ { time ./fd_tables --output_TXT 2> /dev/null; } 2>> textTimes.txt && ls -sh compositeTable.txt >> textTimes.txt`  
`$ { time ./fd_tables --output_binary 2> /dev/null; } 2>> binaryTimes.txt && ls -sh compositeTable.bin >> binaryTimes.txt`

---

### Text Output Times and Sizes

Note, this was run on my computer running Linux.

#### All PIDS

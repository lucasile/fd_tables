# FD Table Tool for Linux. Made for CSCB09

Author: Lucas Ilea

## Documentation

## Compilation and make

To compile to an executable, run

`$ make`

while in the directory of the repository.

If you'd like to remove the executable, run

`$ make clean`

If you'd like to remove the output files if they exist, run

`$ make clean-output`

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

## Observations

Using these two commands, I was able to append to some files to track the time it took to run
the program for each output type, and the corresponding file size.

`$ { time ./fd_tables --output_TXT 2> /dev/null; } 2>> textTimes.txt && ls -sh compositeTable.txt >> textTimes.txt`  
`$ { time ./fd_tables --output_binary 2> /dev/null; } 2>> binaryTimes.txt && ls -sh compositeTable.bin >> binaryTimes.txt`

---

### Text Output Times and Sizes

Note, this was run on my computer running Linux.

#### All PIDS compositeTable.txt

```
0.02s user 0.01s system 90% cpu 0.031 total
116K compositeTable.txt

0.01s user 0.01s system 97% cpu 0.029 total
116K compositeTable.txt

0.01s user 0.02s system 98% cpu 0.029 total
116K compositeTable.txt

0.01s user 0.01s system 98% cpu 0.028 total
116K compositeTable.txt

0.01s user 0.01s system 91% cpu 0.031 total
112K compositeTable.txt

Average Time: 0.0296
Average Size: 115.2K

Standard Deviation of Time: 0.0013416408
```

#### All PIDS compositeTable.bin

```
0.02s user 0.02s system 98% cpu 0.034 total
116K compositeTable.bin

0.02s user 0.01s system 93% cpu 0.028 total
116K compositeTable.bin

0.01s user 0.01s system 97% cpu 0.027 total
116K compositeTable.bin

0.01s user 0.01s system 98% cpu 0.030 total
116K compositeTable.bin

0.02s user 0.01s system 98% cpu 0.029 total
116K compositeTable.bin

Average Time: 0.0296
Average Size: 116K

Standard Deviation of Time: 0.0027018512
```

#### PID 821 (firefox) compositeTable.txt

```
0.00s user 0.00s system 92% cpu 0.002 total
16K compositeTable.txt

0.00s user 0.00s system 93% cpu 0.002 total
16K compositeTable.txt

0.00s user 0.00s system 94% cpu 0.003 total
16K compositeTable.txt

0.00s user 0.00s system 68% cpu 0.003 total
16K compositeTable.txt

0.00s user 0.00s system 79% cpu 0.003 total
16K compositeTable.txt

Average Time: 0.0026
Average Size: 16K

Standard Deviation of Time: 0.00054772256
```

#### PID 821 (firefox) compositeTable.bin

```
0.00s user 0.00s system 95% cpu 0.008 total
16K compositeTable.bin

0.00s user 0.00s system 92% cpu 0.002 total
16K compositeTable.bin

0.00s user 0.00s system 95% cpu 0.003 total
16K compositeTable.bin

0.00s user 0.00s system 77% cpu 0.004 total
16K compositeTable.bin

0.00s user 0.00s system 92% cpu 0.002 total
16K compositeTable.bin

Average Time: 0.0038
Average Size: 16K

Standard Deviation of Time: 0.0024899799
```

---

## Analysis

The time it took to create and write all the information to each file was on average very similar.
The average size of each text file was similar to it's binary counterpart.

I believe the reason this happened is due to text being written as text (ASCII) in binary files
as opposed to other data types. Since we are just writing the string of each process entry to the file,
it is just writing text to the binary file.

Since both files are written to using the exact same datatype, the observations are expected
to be this similar.

This observation can be seen when running `$ cat compositeTable.bin`, where it outputs human-readable
text. The presence of this shows that the text was just being written as plain text in the binary file.

It also makes sense that for a singular PID, the time it takes to run will be lower and the size of the files will
be much lower. This is due to us just looking at a singular PID and writing it's entries, rather than all of them.

For the standard deviations, all of them were so low that we can count these values as accurate and precise.
We can account for higher standard deviation as being normal computer resource spikes and other un-controlled variables.

---

## Code

#### structprocess.h

This header file just includes a simple linked list node struct that holds the information for a single process.  
This information includes, the PID, fd, iNode, symlink, and next node in the linked list.

#### void showTable(procentry \*root, int, int, int, int, int, int, int, int)

This function takes in the root of the linked list, iterates over it and filters out certain nodes depending on whether a PID was specified, and
depending on the arguments provided, routes the information to be printed or written to the respective files via their own functions.

For the four following functions, I will only describe one since they all do the same thing, with different formatting and information provided.

#### void printPerProcessEntry(procentry \*entry)

For the node provided, we print out a formatted string for the `--per-process` flag.

#### void printSystemEntry(procentry \*entry)

#### void printVNodeEntry(procentry \*entry)

#### void printCompositeEntry(procentry \*entry)

#### void writeCompositeToTextFile(FILE \**file, procentry *entry)

Uses fprintf to format and write the text file normally written for a composite entry to the file pointer provided.

#### void writeCompositeToBinaryFile(FILE \**file, procentry *entry)

Uses snprintf to format and write the composite entry to a buffer, where then we use fwrite to write to the binary file. It is assumed the file is opened in binary mode for this function.

#### void printOffendingProcesses(procentry \*root, int threshold)

Iterates through the linked list and prints only the entries where the file descriptor is of greater value than the threshold provided.

#### procentry\* deleteList(procentry \*\*root)

Deletes the linked list by traversing through it and freeing the nodes. Returns NULL to assign to the list head.

#### procentry\* createProcessEntry(int pid, int fd, ino_t iNode, char symlink[256])

Creates a new node by allocating memory using `malloc()` and initializing it's values to the arguments provided.

#### addToEndOfList(procentry \*\*root, int pid, int fd, ino_t iNode, char symlink[256])

Creates a new process entry node using `createProcessEntry()`, traverses to the end of the linked list, and adds it.
We want to add to the end to preserve order of the processes.

#### int getSymlink(char buffer[256], int pid, int fd)

Writes the symlink of a specified pid and fd to the buffer provided by formatting the path string using snprintf, then using `readlink` to get the link. It is
important to note that `readlink` does not null terminate the strings, so we do that with the `linkSize` returned by `readlink`. We then free the temporary path. If this function
returns -1, there was an error.

#### ino_t getINode(int pid, int fd)

Similarily to `getSymlink()`, we format the path string using snprintf, then use `stat` on this path to store it to a buffer stat struct
where we can then get the iNode number with `fileStat.st_ino` and return i and return it.

#### void addProcessEntriesToList(procentry \*\*root, int pid)

In this function we open the path of the specified pid to their fd directory, `/proc/pid/fd/` and then open the directory using
`opendir()`

From here we can declare a `dirent` entry, then iterate through it until it is NULL to get all the directories in the path.
We validate the directories by using the `validPath()` function and checking if the fd is a positive integer (valid).
We then use `getSymlink()` on the pid and fd. If this returns -1, we continue out of this iteration because we have an error.
Similarily, we do the same with `getINode()`

Once we have all the necessary information for a process node, we then call `addToEndOfList()` with these parameters.

Afterwards, we close the directory stream with `closedir()`

#### procentry\* getAvailableProcesses()

Simiarily to the above function, we use `opendir()` on the path `/proc/`.
We get the uid of the user running the program by `getuid()` and save it in a variable for later use in the loop.
We also setup the root of the linked list and assign it to null.

We then iterate through all the directories in this path using the same method as above, but this time we are trying to find all the PIDs of processes.

We validate using `validPath()` and then check if the pid is a valid positive integer.

Afterwards, we snprintf the path of `/proc/pid` with the pid that we found to a buffer, then use `stat()` to be able to  
extract the owner's uid of the directory we are looking at, and compare it to the user's uid. If they are not equal, we continue out of this iteration
since we won't have valid permissions to do what we want with this process.

Finally, we call `addProcessEntriesToList()` to add all the entries for the pid we just found and validated to the list.

Once we are done, we close the directory stream with `closedir()` and return the root of the linked list.

#### int validPath(char\*)

This simply checks the string provided and whether it starts with a '.'. If it does, then we return 0 since it isn't a valid path (either '.' or '..') from when we
iterate over the directories.

#### size_t maxPathSize(int)

This is a function that simplifies how big we should make the buffer for the path sizes depending on the size of the pid provided.
It just takes the base 10 logarithm of the pid and casts it to an int to get the number of digits - 1 that the pid is. Then we add 1 to account for the casting, and 15 for some extra room to be safe.

#### composeArgs(int\*)

This function simply just takes in the CLA argument array provided by the user and routes them to the proper functionality.
For example, if the user wanted the per process table, it will call the proper function with the arguments to do that.

It also sets up the root of the linked list, determines whether we want to get all of the pids or just one, and calls the appropriate functions above to set it up.

For threshold, it also makes sure that if we have the list for just one pid, we get all of the pids when we need to get the offending processes.
It does this by deleting the original list, and getting it again but this time for every process.

Finally it calls `deleteList` to free all the memory.

#### setFlags(int *flags, int argc, char *argv[])

This function simply just iterates through all the arguments provided, splits them using `strtok()` with the delimiter of `=` for arguments that
allow the user to provide dynamic values, and then compares the strings to set the necessary flags in the array.

After checking whether the current argument is valid, we check whether it is the first argument with `i == 1`, and if so then it has the opportunity to be a positional argument.
Then we check if this argument is a valid pid candidate by seeing if it is a positive integer, and if it is we turn off `pidAll` in flags and set the pid element to the specified argument.

This function works the same as the `setFlags` function in my Assignment 1, system_info.

Finally at the end, we have a big boolean expression to check whether no arguments were specified, a pid was specified by itself, with the output file flags, or with just the output file flags.
If this is the case, we set the default setting to just print the composite table, since no other options were specified.

#### main

In the main function we just set up the flags array, where each element represents a different possible argument, call our CLA parser using `setFlags()` to set the array's values, then run `composeArgs()`
to run the program with the specified arguments.

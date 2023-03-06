#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include "structprocess.h"

// display functions

void showTable(procentry* root, int, int, int, int, int, int, int, int);

void printEntry(struct dirent *entry, int, int, int, int, int);

void printPerProcessEntry(procentry *entry);
void printSystemEntry(procentry *entry);
void printVNodeEntry(procentry *entry);
void printCompositeEntry(procentry *entry);

void printOffendingProcesses(procentry *root, int threshold);

// managing linked list functions

procentry* deleteList(procentry **root);
procentry* createProcessEntry(int pid, int fd, ino_t iNode, char symlink[256]);
void addToEndOfList(procentry **root, int pid, int fd, ino_t iNode, char symlink[256]);

// getting needed values functions

int getSymlink(char buffer[256], int pid, int fd);
ino_t getINode(int pid, int fd);

// reading a directory stream and parsing/validating functions

void addProcessEntriesToList(procentry **root, int pid);
procentry* getAvailableProcesses();

// validation and safety functions

int validPath(char*);
size_t maxPathSize(int);

// create and output file
void writeCompositeToTextFile(FILE **file, procentry *entry);
void writeCompositeToBinaryFile(FILE **file, procentry *entry);

// take arguments and route them properly 

void composeArgs(int*);

// argument handling
int setFlags(int*, int, char**);

int main(int argc, char *argv[]) {

  int flags[9] = {
    0, //per-process
    0, //system-wide
    0, //Vnodes
    0, //composite
    -1, //threshold=X
    1, //pid all
    0, //pid if not all
    0, //output txt
    0, //output binary
  };

  // take the CLA and parse them. if there is an error it will return 0 and we exit the program
  
  if (setFlags(flags, argc, argv) == 0) {
    return 0;
  }

  composeArgs(flags);

  return 0;
}

void showTable(procentry* root, int pidAll, int pid, int perProcess, int systemWide, int vNodes, int composite, int outputTXT, int outputBinary) {

  procentry *currentEntry = root;

  FILE *fileTXT;
  FILE *fileBinary;

  if (outputTXT == 1) {
    fileTXT = fopen("compositeTable.txt", "w");
  }

  if (outputBinary == 1) {
    fileBinary = fopen("compositeTable.bin", "w:b");
  }

  // iterate over the linked list 

  while (currentEntry != NULL) {

    // if we only care about one pid, we compare against the one specified
    if (pidAll == 0) {
      if (currentEntry -> pid != pid) {
        currentEntry = currentEntry -> next;
        continue;
      }
    }

    // print the proper entries for the table
    if (perProcess == 1) {
      printPerProcessEntry(currentEntry);
    } else if (systemWide == 1) {
      printSystemEntry(currentEntry);
    } else if (vNodes == 1) {
      printVNodeEntry(currentEntry);
    } else if (composite == 1) {
      printCompositeEntry(currentEntry);
    }

    if (outputTXT == 1) {
      writeCompositeToTextFile(&fileTXT, currentEntry);
    }

    if (outputBinary == 1) {
      writeCompositeToBinaryFile(&fileBinary, currentEntry);
    }

    // go to next entry 
    currentEntry = currentEntry -> next;

  }

  if (outputTXT == 1) {
    fclose(fileTXT);
  }

  if (outputBinary == 1) {
    fclose(fileBinary);
  }

}

void printPerProcessEntry(procentry *entry) {
  // print pid and fd for --per-process
  printf("    %d\t\t%d\n", entry -> pid, entry -> fd);
}

void printSystemEntry(procentry *entry) {
  // print pid, fd, and the fd's symlink for --systemWide
  printf("    %d\t\t%d        %s\n", entry -> pid, entry -> fd, entry -> symlink);
} 

void printVNodeEntry(procentry *entry) {
  // print the fd and the inode associated for --Vnodes
  // we use a %ju and cast to uintmax_t because we don't know the size of iNode as it is of type ino_t. If the value
  // exceeds the integer range for example (possible through testing), we will get junk values if we just print using %d.
  printf("    %d\t\t%ju\n", entry -> fd, (uintmax_t) entry -> iNode);
}

void printCompositeEntry(procentry *entry) {
  // print the pid, fd, inode, and fd's symlink for --composite
  printf("    %d\t\t%d\t\t%ju\t\t%s\n", entry -> pid, entry -> fd, (uintmax_t) entry -> iNode, entry -> symlink);
}

// precon: file opened with w 
void writeCompositeToTextFile(FILE **file, procentry *entry) {
  // note: file already opened
  fprintf(*file, "    %d\t\t%d\t\t%ju\t\t%s\n", entry -> pid, entry -> fd, (uintmax_t) entry -> iNode, entry -> symlink);
}

// precon: file opened with wb 
void writeCompositeToBinaryFile(FILE **file, procentry *entry) {
  // note: file already opened
  char* buffer = malloc(512 * sizeof(char));
  snprintf(buffer, 512 * sizeof(char), "    %d\t\t%d\t\t%ju\t\t%s\n", entry -> pid, entry -> fd, (uintmax_t) entry -> iNode, entry -> symlink);
  fwrite(&buffer, 512 * sizeof(char), 1, *file);
  free(buffer);
}

void printOffendingProcesses(procentry *root, int threshold) {

  procentry *currentEntry = root;

  // iterate through linked list
  while (currentEntry != NULL) {

    // print out any processes that have fd's larger than the specified threshold
    if (currentEntry -> fd > threshold) {
      printf("%d:(%d), ", currentEntry -> pid, currentEntry -> fd);
    }

    currentEntry = currentEntry -> next;

  }

}

int validPath(char* filename) {
  // to make sure we don't include '.' and '..' when we look in /proc/ and /proc/pid/fd/
  if (filename[0] == '.') {
    return 0;
  }
  return 1;
}

size_t maxPathSize(int pid) {
  // returns a safe max path size for a given pid given we want to look at /proc/pid/fd/
  // log10f of pid casted to int will give the number of digits in the pid
  return ((int) log10f((float) pid)) + 1 + 15; 
}

int getSymlink(char buffer[256], int pid, int fd) {

  // set a max size for the symlink path to 256
  size_t maxSize = 256;

  // allocate memory for the path 
  char* path = malloc(maxSize * sizeof(char));
  // set the path of the file descriptor to path
  snprintf(path, maxSize, "/proc/%d/fd/%d", pid, fd);

  ssize_t linkSize = readlink(path, buffer, 256);

  // use readlink to populate the buffer argument with the symlink associated to the fd path 
  if (linkSize == -1) {
    // if we get an error we free path and return out
    free(path);
    return -1;
  }

  // cap linksize
  if (linkSize > 255) {
    linkSize = 255;
  }
  
  // null terminate it 
  buffer[linkSize] = '\0';

  // free path since it is allocated
  free(path);

  return 0;
}

ino_t getINode(int pid, int fd) {

  // max path size is 256 as above
  size_t maxSize = 256;

  // allocate memory for the path to the fd
  char* path = malloc(maxSize * sizeof(char));
  // set the path of the file descriptor to path
  snprintf(path, maxSize, "/proc/%d/fd/%d", pid, fd);

  struct stat fileStat;

  if (stat(path, &fileStat) == -1) {
    return -1;
  }

  // return the inode number with type ino_t from the filestat 
  return fileStat.st_ino;

}

void addProcessEntriesToList(procentry **root, int pid) {

  // use maxPathSize to get the max size the path should have
  size_t maxSize = maxPathSize(pid) * sizeof(char);

  char* path = malloc(maxSize);
  // set the fd directory path to path
  snprintf(path, maxSize, "/proc/%d/fd/", pid);

  // open the directory
  DIR *directory = opendir(path);

  // we have an issue here. possibly mismatched permissions since we already check against the owner of the directory. prune these ones out
  if (directory == NULL) {
    //printf("/proc/%d/fd/ ", pid);
    //perror("directory could not be opened");
    free(path);
    return;
  } 

  // free the path after opening the directory
  free(path);

  struct dirent *entry;   

  // use directory streams to get all directories in the fd dir 
  while ((entry = readdir(directory)) != NULL) {

    // validation
    if (validPath(entry -> d_name) == 0) {
      continue;
    }

    // turn fd into int
    int fd = atoi(entry -> d_name);

    // make sure fd is valid. we check whether it is <= 0 and if it's first character is 0 because atoi returns 0 if
    // it is invalid, but 0 is also a valid fd
    if (fd <= 0 && (entry -> d_name)[0] != '0') {
      perror("ERR:InvalidFD");
      continue;
    }

    // declare a char[256] to hold the fd symlink
    char symlink[256];

    // use getSymLink above to populate symlink
    if (getSymlink(symlink, pid, fd) == -1) {
      // if we have an issue and can't get it for whatever reason, just skip this entry
      continue;
    }

    // declare the iNode variable
    ino_t iNode;

    // use getINode to set iNode.
    if ((iNode = getINode(pid, fd)) == -1) {
      // if we have an issue skip entry
      continue;
    }

    // add this new entry to the end of the list to preserve pid order.
    addToEndOfList(root, pid, fd, iNode, symlink);

  }

  // close dir stream 
  closedir(directory);

}

procentry* getAvailableProcesses() {

  // open a directory stream
  DIR *directory = opendir("/proc/");

  if (directory == NULL) {
    // shouldn't happen, but for debugging in case permissions are wrong on the system
    perror("/proc/ directory could not be opened.");
  }

  struct dirent *entry;

  // get running process' user id to compare later
  uid_t uid = getuid();

  // setup the head of the linked list
  procentry *processEntryRoot = NULL;

  while ((entry = readdir(directory)) != NULL) {

    // validate using validPath
    if (validPath(entry -> d_name) == 0) {
      continue;
    }
   
    // validate pid as we validated fd above in addProcessEntriesToList
    int pid = atoi(entry -> d_name);

    if (pid <= 0 && (entry -> d_name)[0] != '0') {
      continue;
    }

    // setup safe max size for path string
    size_t maxSize = maxPathSize(pid) * sizeof(char);

    // allocate memory for path string
    char* path = malloc(maxSize);
    // set the path to /proc/currpid
    snprintf(path, maxSize, "/proc/%d", pid);

    struct stat fileStat;
    
    // stat the file to retrieve the owner's uid 
    if (stat(path, &fileStat) == -1) {
      // if we can't stat it for whatever reason, just go next
      perror("Could not stat file");
      free(path);
      continue;
    }

    // compare the file's owner id to our user id, if it is equal then we can go on without permission issues (under normal circumstances)
    if (fileStat.st_uid != uid) {
      free(path);
      continue;
    }

    free(path);

    // we have a valid process

    // call addProcessEntriesToList to add all the fd entries for the current pid to the list 
    addProcessEntriesToList(&processEntryRoot, pid);
    
  }

  // close the directory stream 
  closedir(directory);

  // return the head of the list 
  return processEntryRoot;
}

void composeArgs(int *flags) {

  // list all args for ease of use
  int perProcess = flags[0];
  int systemWide = flags[1];
  int vNodes = flags[2];
  int composite = flags[3];
  int threshold = flags[4];
  int pidAll = flags[5];
  int pidSelected = flags[6];
  int outputTXT = flags[7];
  int outputBinary = flags[8];

  // setup the head of the linked list 
  procentry* processEntryRoot = NULL;

  if (pidAll == 1) {
    // if a pid wasn't specified, we need to get all of them
    processEntryRoot = getAvailableProcesses();
  } else {
    // if not, we just add the entries for the pid specified
    addProcessEntriesToList(&processEntryRoot, pidSelected);
  }

  // the following if statements just print the tables depending on the CLA specified
  if (perProcess == 1) {
    printf("|====PID=======FD====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 1, 0, 0, 0, outputTXT, outputBinary);
    printf("\n");
  }

  if (systemWide == 1) {
    printf("|====PID=======FD=======LINK====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 0, 1, 0, 0, outputTXT, outputBinary);
    printf("\n");
  }

  if (vNodes == 1) {
    printf("|====FD=======INODE====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 0, 0, 1, 0, outputTXT, outputBinary);
    printf("\n");
  }

  if (composite == 1) {
    printf("|====PID=======FD=======INODE=======LINK====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 0, 0, 0, 1, outputTXT, outputBinary);
    printf("\n");
  }

  if (threshold != -1) {

    // if we have a threshold set, we need to iterate through every process entry, not just the one selected

    if (pidAll == 0) {
      // if a process was specified, we have to delete and remake the linked list with every process to be able to filter them
      processEntryRoot = deleteList(&processEntryRoot);
      processEntryRoot = getAvailableProcesses();
    }

    // if no process was specified, our linked list is already how we want it.

    // print the offending processes using printOffendingProcceses
    printf("## Offending Processes:\n");
    printOffendingProcesses(processEntryRoot, threshold);
    printf("\n");

  }

  // once we are done, delete the list to free memory
  processEntryRoot = deleteList(&processEntryRoot);

}

int setFlags(int *flags, int argc, char *argv[]) {

  // iterate through all the possible CLA
  for (int i = 1; i < argc; i++) {

    // split the flag at the = sign in case one of the arguments is threshold.
    char *flag = strtok(argv[i], "=");

    // compare flag to all the arguments we want, and set the respective element in the array on
    if (strcmp(flag, "--per-process") == 0) {
      flags[0] = 1;
    } else if (strcmp(flag, "--systemWide") == 0) {
      flags[1] = 1;
    } else if (strcmp(flag, "--Vnodes") == 0) {
      flags[2] = 1;
    } else if (strcmp(flag, "--output_TXT") == 0) {
      flags[7] = 1;
    } else if (strcmp(flag, "--output_binary") == 0) {
      flags[8] = 1;
    } else if (strcmp(flag, "--composite") == 0) {
      flags[3] = 1;
    } else if (strcmp(flag, "--threshold") == 0) {

      // if we get threshold, we need to check if the value specified is valid
      
      flag = strtok(NULL, "=");

      // flag now points to the value specified after =
  
      // validate this value
      if (flag == NULL) {
        printf("Threshold argument invalid.\n");
        return 0;
      }

      // convert this to base 10 int
      
      int threshold = strtol(flag, NULL, 10);

      // validate it 
      if (threshold > 0) {

        flags[4] = threshold;

      } else {
        printf("Threshold argument invalid\n");
        return 0;
      }
    // positional argument here
    } else if (i == 1) { // pid specified must be first argument so we check if i == 1
      
      // convert to int
      int pid = atoi(flag);

      // check if it's valid
      if (pid <= 0) {
        printf("Invalid PID specified\n");
        return 0;
      }

      // since it was specified, we turn pid-all off.
      flags[5] = 0;
      flags[6] = pid;

    } else {
      printf("Invalid argument(s) specified. Please refer to documentation.\n");
      return 0;
    }

  }

  // if no flags other than position arguments specified and outputting files, set it to print the composite table
  if (argc <= 1 || (argc == 2 && flags[5] == 0) || (argc == 2 && (flags[7] == 1|| flags[8] == 1)) || 
      (argc == 3 && (flags[7] == 1 && flags[8] == 1))) {
    // default composite = 1,
    flags[3] = 1;
  }
  
  return 1;

}

// below are standard linked list functions

procentry* deleteList(procentry **root) {

  // traverse the list freeing each node

  procentry *traverse = *root;
  procentry *next = NULL;

  while (traverse != NULL) {
    next = traverse -> next;
    free (traverse);
    traverse = next;
  }

  return NULL;

}

void addToEndOfList(procentry **root, int pid, int fd, ino_t iNode, char symlink[256]) {

  procentry *processEntry = createProcessEntry(pid, fd, iNode, symlink);

  // if the head is null, set it to the new node and exit the function

  if (*root == NULL) {
    *root = processEntry;
    return;
  }

  // else get to the end of the list, where we will then add this node

  procentry *traverse = *root;

  while (traverse -> next != NULL) {
    traverse = traverse -> next;
  }

  traverse -> next = processEntry;
}

procentry* createProcessEntry(int pid, int fd, ino_t iNode, char symlink[256]) {

  // allocate memory for a new entry

  procentry *processEntry = (procentry *) malloc(sizeof(procentry));

  // set it's values to the specified args
  processEntry -> pid = pid;
  processEntry -> fd = fd;
  processEntry -> iNode = iNode;

  //set it's next to null since we will only append this to the end of the list
  processEntry -> next = NULL;

  // copy the symlink to the string in the struct 
  strncpy(processEntry -> symlink, symlink, 256 * sizeof(char));

  // return the new node
  return processEntry;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "structprocess.h"

void showTable(procentry* root, int, int, int, int, int, int);

void printEntry(struct dirent *entry, int, int, int, int, int);

void printPerProcessEntry(procentry *entry);
void printSystemEntry(procentry *entry);
void printVNodeEntry(procentry *entry);
void printCompositeEntry(procentry *entry);

procentry* deleteList(procentry *root);
procentry* createProcessEntry(int pid, int fd, int symlinkInode, char symlink[256]);
void addToEndOfList(procentry *root, int pid, int fd, int symlinkInode, char symlink[256]);

int getSymlink(char buffer[256], int pid, int fd);
int getINode(char path[256]);

void addProcessEntriesToList(procentry* root, int pid);
procentry* getAvailableProcesses();

int validPath(char*);
size_t maxPathSize(int);

void composeArgs(int*);

// argument handling
int setFlags(int*, int, char**);

int main(int argc, char *argv[]) {

  int flags[7] = {
    0, //per-process
    0, //system-wide
    0, //Vnodes
    0, //composite
    0, //threshold=X
    1, //pid all
    0, //pid if not all
  };

  if (setFlags(flags, argc, argv) == 0) {
    return 0;
  }

  composeArgs(flags);

  return 0;
}

void showTable(procentry* root, int pidAll, int pid, int perProcess, int systemWide, int vNodes, int composite) {

  procentry *currentEntry = root;

  while (currentEntry != NULL) {

    if (pidAll == 0) {
      if (currentEntry -> pid != pid) {
        currentEntry = currentEntry -> next;
        continue;
      }
    }

    if (perProcess == 1) {
      printPerProcessEntry(currentEntry);
    } else if (systemWide == 1) {
      printSystemEntry(currentEntry);
    } else if (vNodes == 1) {
      printVNodeEntry(currentEntry);
    } else if (composite == 1) {
      printCompositeEntry(currentEntry);
    }

    currentEntry = currentEntry -> next;

  }

}

void printPerProcessEntry(procentry *entry) { 
  printf("    %d        %d    \n", entry -> pid, entry -> fd);
}

void printSystemEntry(procentry *entry) {

   printf("    %d        %d        %s\n", entry -> pid, entry -> fd, entry -> symlink);
} 

void printVNodeEntry(procentry *entry) {

}

void printCompositeEntry(procentry *entry) {

}

int validPath(char* filename) {
  if (filename[0] == '.') {
    return 0;
  }
  return 1;
}

size_t maxPathSize(int pid) {
  return ((int) log10f((float) pid)) + 1 + 15; 
}

int getSymlink(char buffer[256], int pid, int fd) {

  size_t maxSize = 256;

  char* path = malloc(maxSize);
  snprintf(path, maxSize, "/proc/%d/fd/%d", pid, fd);

  if (readlink(path, buffer, 256) == -1) {
    free(path);
    return -1;
  }

  free(path);

  return 0;
}

int getINode(char path[256]) {

  struct stat fileStat;

  if (stat(path, &fileStat) == -1) {
    return -1;
  }

  return fileStat.st_ino;

}

void addProcessEntriesToList(procentry* root, int pid) {

  size_t maxSize = maxPathSize(pid) * sizeof(char);

  char* path = malloc(maxSize);
  snprintf(path, maxSize, "/proc/%d/fd/", pid);


  DIR *directory = opendir(path);

  if (directory == NULL) {
    //printf("/proc/%d/fd/ ", pid);
    //perror("directory could not be opened");
    free(path);
    return;
  } 

  free(path);

  struct dirent *entry;   

  while ((entry = readdir(directory)) != NULL) {

    // validation

    if (validPath(entry -> d_name) == 0) {
      continue;
    }

    int fd = atoi(entry -> d_name);

    if (fd <= 0 && (entry -> d_name)[0] != '0') {
      perror("ERR:InvalidFD");
      continue;
    }

    char symlink[256];

    if (getSymlink(symlink, pid, fd) == -1) {
      printf("hello\n");
      continue;
    }

    int iNode;

    if ((iNode = getINode(symlink)) == -1) {
      printf("%s\n", symlink);
      continue;
    }

    if (root == NULL) {
      printf("pid: %d, fd: %d, inode: %d\n", pid, fd, iNode);
      root = createProcessEntry(pid, fd, iNode, symlink);
    } else {
      addToEndOfList(root, pid, fd, iNode, symlink);
    }

  }

  closedir(directory);

}

procentry* getAvailableProcesses() {

  DIR *directory = opendir("/proc/");

  if (directory == NULL) {
    perror("/proc/ directory could not be opened.");
  }

  struct dirent *entry;

  uid_t uid = getuid();

  procentry *processEntryRoot = NULL;

  while ((entry = readdir(directory)) != NULL) {

    if (validPath(entry -> d_name) == 0) {
      continue;
    }
   
    int pid = atoi(entry -> d_name);

    if (pid <= 0 && (entry -> d_name)[0] != '0') {
      continue;
    }

    size_t maxSize = maxPathSize(pid) * sizeof(char);

    char* path = malloc(maxSize);
    snprintf(path, maxSize, "/proc/%d", pid);

    struct stat fileStat;
    
    if (stat(path, &fileStat) == -1) {
      perror("Could not stat file");
      free(path);
      continue;
    }

    if (fileStat.st_uid != uid) {
      free(path);
      continue;
    }

    free(path);

    // we have a valid process

    addProcessEntriesToList(processEntryRoot, pid);
    
  }

  closedir(directory);

  return processEntryRoot;
}

void composeArgs(int *flags) {

  int perProcess = flags[0];
  int systemWide = flags[1];
  int vNodes = flags[2];
  int composite = flags[3];
  int threshold = flags[4];
  int pidAll = flags[5];
  int pidSelected = flags[6];

 // for (int i = 0; i < 7; i++) {
 //    printf("%d\n", flags[i]);
 //  }

  procentry* processEntryRoot = NULL;

  if (pidAll == 1) {
    processEntryRoot = getAvailableProcesses();
  } else {
    addProcessEntriesToList(processEntryRoot, pidSelected);
  }

  if (perProcess == 1) {
    printf("|====PID=======FD====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 1, 0, 0, 0);
    printf("\n");
  }

  if (systemWide == 1) {
    printf("|====PID=======FD=======LINK====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 0, 1, 0, 0);
    printf("\n");
  }

  if (vNodes == 1) {
    printf("|====PID=======INODE====|\n");
    showTable(processEntryRoot, pidAll, pidSelected, 0, 0, 1, 0);
  }

  processEntryRoot = deleteList(processEntryRoot);

}

int setFlags(int *flags, int argc, char *argv[]) {

   for (int i = 1; i < argc; i++) {

    char *flag = strtok(argv[i], "=");

    if (strcmp(flag, "--per-process") == 0) {
      flags[0] = 1;
    } else if (strcmp(flag, "--systemWide") == 0) {
      flags[1] = 1;
    } else if (strcmp(flag, "--Vnodes") == 0) {
      flags[2] = 1;
    } else if (strcmp(flag, "--composite") == 0) {
      flags[3] = 1;
    } else if (strcmp(flag, "--threshold") == 0) {

      flag = strtok(NULL, "=");

      if (flag == NULL) {
        perror("ERR:threshold\n");
        return 0;
      }

      int threshold = strtol(flag, NULL, 10);

      if (threshold > 0) {

        flags[4] = threshold;

      } else {
        perror("ERR:threshold\n");
        return 0;
      }
    // positional argument here
    } else if (i == 1) {
      
      int pid = atoi(flag);

      if (pid <= 0) {
        perror("ERR:pidInvalid\n");
        return 0;
      }

      // not pid-all
      flags[5] = 0;
      flags[6] = pid;

    } else {
      perror("ERR:invalidArg\n");
      return 0;
    }

  }

   if (argc <= 1 || (argc == 2 && flags[5] == 0)) {
    // default composite = 1,
    flags[3] = 1;
    return 1;
  }
  
  return 1;

}

procentry* deleteList(procentry *root) {

  procentry *traverse = root;
  procentry *next = NULL;

  while (traverse != NULL) {
    next = traverse -> next;
    free (traverse);
    traverse = next;
  }

  return NULL;

}

void addToEndOfList(procentry* root, int pid, int fd, int symlinkInode, char symlink[256]) {

  procentry *processEntry = createProcessEntry(pid, fd, symlinkInode, symlink);
  
  procentry *traverse = root;

  while (traverse -> next != NULL) {
    traverse = traverse -> next;
  }

  traverse -> next = processEntry;
}

procentry* createProcessEntry(int pid, int fd, int symlinkInode, char symlink[256]) {

  procentry *processEntry = (procentry *) malloc(sizeof(procentry));

  processEntry -> pid = pid;
  processEntry -> fd = fd;
  processEntry -> symlinkInode = symlinkInode;
  processEntry -> next = NULL;

  strncpy(processEntry -> symlink, symlink, 256 * sizeof(char));

  return processEntry;
}

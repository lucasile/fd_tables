#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void showTableEntry(int*, int, int, int, int, int);

void printEntry(struct dirent *entry, int, int, int, int, int);

void printPerProcessEntry(struct dirent *entry, int);
void printSystemEntry(struct dirent *entry, int);
void printVNodeEntry(struct dirent *entry, int);
void printCompositeEntry(struct dirent *entry, int);

int validFd(char*);
void setPath(char*, int, size_t);
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

void showTableEntry(int* pids, int numPID, int perProcess, int systemWide, int vNodes, int composite) {

  if (perProcess + systemWide + vNodes + composite == 0) {
    return;
  }

  for (int i = 0; i < numPID; i++) {
  
    int pid = pids[i];

    size_t maxSize = maxPathSize(pid);

    char* path = malloc(maxSize * sizeof(char));
    setPath(path, pid, maxSize);

    DIR *directory = opendir(path);

    if (directory == NULL) {
      perror("directory could not be opened");
    } 

    struct dirent *entry;   


    while ((entry = readdir(directory)) != NULL) {
      if (validFd(entry -> d_name) == 0) {
        continue;
      }
      printEntry(entry, pid, perProcess, systemWide, vNodes, composite);
    }

    closedir(directory);
    free(path);
  }

}

void printEntry(struct dirent *entry, int pid, int perProcess, int systemWide, int vNodes, int composite) {
  if (perProcess == 1) {
    printPerProcessEntry(entry, pid);
  } else if (systemWide == 1) {
    printSystemEntry(entry, pid);
  } else if (vNodes == 1) {
    printVNodeEntry(entry, pid);
  } else if (composite == 1) {
    printCompositeEntry(entry, pid);
  }
}

void printPerProcessEntry(struct dirent *entry, int pid) { 
  printf("%d    ,    %s\n", pid, entry -> d_name);
}

void printSystemEntry(struct dirent *entry, int pid) {

}

void printVNodeEntry(struct dirent *entry, int pid) {

}

void printCompositeEntry(struct dirent *entry, int pid) {

}

int validFd(char* filename) {
  if (filename[0] == '.') {
    return 0;
  }
  return 1;
}

void setPath(char* path, int pid, size_t length) {
  snprintf(path, length * sizeof(char), "/proc/%d/fd/", pid);
}

size_t maxPathSize(int pid) {
  return ((int) log10f((float) pid)) + 1 + 15; 
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

  int pids[1] = {
    0
  };

  pids[0] = getpid();

  showTableEntry(pids, 1, 1, 0, 0, 0);

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


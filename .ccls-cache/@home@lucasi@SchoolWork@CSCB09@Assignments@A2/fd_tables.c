#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void showProcessEntry(int*, int, int, int, int, int);

void printEntry(struct dirent *entry, int, int, int, int, int);

void printPerProcessEntry(struct dirent *entry, int);
void printSystemEntry(struct dirent *entry, int);
void printVNodeEntry(struct dirent *entry, int);
void printCompositeEntry(struct dirent *entry, int);

int getAvailableProcesses(int**);

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

void showProcessEntry(int* pids, int numPID, int perProcess, int systemWide, int vNodes, int composite) {

  if (perProcess + systemWide + vNodes + composite == 0) {
    return;
  }

  for (int i = 0; i < numPID; i++) {
  
    int pid = pids[i];

    size_t maxSize = maxPathSize(pid) * sizeof(char);

    char* path = malloc(maxSize);
    snprintf(path, maxSize, "/proc/%d/fd/", pid);


    DIR *directory = opendir(path);

    if (directory == NULL) {
      perror("/proc/pid/fd/ directory could not be opened");
      free(path);
      continue;
    } 

    free(path);

    struct dirent *entry;   


    while ((entry = readdir(directory)) != NULL) {
      if (validPath(entry -> d_name) == 0) {
        continue;
      }
      printEntry(entry, pid, perProcess, systemWide, vNodes, composite);
    }

    closedir(directory);
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
  printf("|    %d    |    %s    |\n", pid, entry -> d_name);
}

void printSystemEntry(struct dirent *entry, int pid) {

}

void printVNodeEntry(struct dirent *entry, int pid) {

}

void printCompositeEntry(struct dirent *entry, int pid) {

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

int getAvailableProcesses(int** pids) {

  DIR *directory = opendir("/proc/");

  if (directory == NULL) {
    perror("/proc/ directory could not be opened.");
  }

  struct dirent *entry;

  uid_t uid = getuid();

  int count = 1;
  *pids = malloc(count * sizeof(int));


  while ((entry = readdir(directory)) != NULL) {

    if (validPath(entry -> d_name) == 0) {
      continue;
    }
   
    int pid = atoi(entry -> d_name);

    if (pid <= 0) {
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

    *pids = realloc(*pids, count * sizeof(int));
    (*pids)[count - 1] = pid;

    count++;
    
  }

  closedir(directory);

  return count - 1;
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

  int* pids;

  int numPID;

  if (pidAll == 1) {
    numPID = getAvailableProcesses(&pids);
  }

  printf("pid:%d\n", pids[0]);

  if (perProcess == 1) {
    printf("|====PID=========FD====|\n");
    showProcessEntry(pids, numPID, 1, 0, 0, 0);
  }

  if (pidAll == 1) {
    free(pids);
  }

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


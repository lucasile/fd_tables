#include <sys/stat.h>

typedef struct ProcessEntry {
  int pid;
  int fd;
  ino_t iNode;
  char symlink[256];
  struct ProcessEntry *next;
} procentry;


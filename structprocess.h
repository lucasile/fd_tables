typedef struct ProcessEntry {
  int pid;
  int fd;
  int symlinkInode;
  char symlink[256];
  struct ProcessEntry *next;
} procentry;


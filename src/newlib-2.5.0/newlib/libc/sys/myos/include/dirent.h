#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <stddef.h>
#include <stdint.h>

struct dirent {
  uint32_t d_ino;
  char d_name[32];
};

struct __dirstream {
  int fd;
  int buf_pos;
  int buf_end;
  char buffer[4096];
  struct dirent current; // To store the result for readdir
};

// Map DIR to your struct
typedef struct __dirstream DIR;

int closedir(DIR *dir);
DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);

#endif

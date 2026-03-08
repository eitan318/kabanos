#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <stddef.h>
#include <stdint.h>

struct dirent {
  uint32_t d_ino;
  char d_name[32];
}

;
typedef struct {
  int fd;
  int buf_pos;
  int buf_end;
  char buffer[1024];
  struct dirent current;
} DIR;

#endif

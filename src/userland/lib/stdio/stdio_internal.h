#pragma once
#include "stddef.h"

typedef struct _IO_FILE {
  int fd;
  char *buf;
  size_t buf_size;
  size_t buf_pos;
} FILE;

#define BUFSIZ 0x10000

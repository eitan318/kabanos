#pragma once
#include "stddef.h"

typedef struct _IO_FILE FILE;

typedef struct _IO_FILE {
  int fd;
  char *buf;
  FILE *next;
  size_t buf_size;
  size_t buf_pos;

} FILE;

#define BUFSIZ 0x10000

extern FILE *_open_streams_head;

void stdio_init();

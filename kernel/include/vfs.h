#pragma once
#include <stddef.h>
#include <stdint.h>

typedef int fd_t;

enum VFS_FD {
  VFS_FD_STDIN,
  VFS_FD_STDOUT,
  VFS_FD_STDERR,
  VFS_FD_DEBUG,
};

size_t pvfs_write(int fd, uint8_t *buf, size_t size);

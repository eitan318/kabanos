#pragma once
#include "fs/vfs.h"
#include "klib/errno.h"
#include "klib/unistd.h"

long sys_open(const char *pathname, int flags);
long sys_close(int fd);
long sys_lseek(int fd, off_t offset, int whence);
long sys_stat(int fd, fstat_t *user_stat);

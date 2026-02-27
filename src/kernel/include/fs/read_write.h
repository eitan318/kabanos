#pragma once
#include "device.h"
#include "fs/vfs.h"
#include "klib/errno.h"

long sys_read(int fd, char *user_buf, size_t count);
long sys_write(int fd, const char *buf, size_t size);
long sys_iter_dir(int fd, VDirEntry *dentry, int count);

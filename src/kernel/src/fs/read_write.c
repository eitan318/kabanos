#pragma once
#include "device.h"
#include "fs/vfs.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "klib/unistd.h"
#include "ksys/stat.h"

long sys_read(int fd, char *buf, size_t count) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -EBADF;

  file_t *f = g_fd_table[fd];
  if (f->f_ops && f->f_ops->read) {
    ssize_t ret = f->f_ops->read(f, buf, count);
    if (ret > 0)
      f->pos += ret;
    return ret;
  }

  return -EINVAL;
}

long sys_write(int fd, const char *buf, size_t len) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -EBADF;

  file_t *f = g_fd_table[fd];

  if (f->f_ops && f->f_ops->write) {
    return f->f_ops->write(f, buf, len);
  }

  return -EINVAL;
}

long sys_iter_dir(int fd, VDirEntry *dentry, int count) {
  if (!dentry || count <= 0)
    return -EINVAL;

  return vfs_iter_dir(fd, dentry, count);
}

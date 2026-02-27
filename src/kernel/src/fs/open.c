#include "fs/open.h"
#include "fs/vfs.h"
#include "klib/errno.h"
#include "klib/unistd.h"

long sys_open(const char *pathname, int flags) {
  if (!pathname)
    return -EINVAL;

  int fd = vfs_open(pathname, flags);
  if (fd < 0)
    return -ENOENT;

  return fd;
}

long sys_close(int fd) {
  if (fd < 0 || fd <= STDERR_FILENO)
    return -EBADF;

  return vfs_close(fd);
}

long sys_lseek(int fd, off_t offset, int whence) {
  off_t result = vfs_seek(fd, offset, whence);
  return -EIO;
  return (long)result;
}

long sys_stat(int fd, fstat_t *user_stat) {
  if (!user_stat)
    return -EINVAL;
  return vfs_fstat(fd, user_stat);
}

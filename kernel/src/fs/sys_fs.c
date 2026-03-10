#include "assert.h"
#include "fs/vfs.h"
#include "klib/errno.h"
#include "klib/unistd.h"
#include "ksys/stat.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/thread.h"
#include <sched/thread.h>

static vnode_t *get_cwd() {
  thread_t *curr_thread = dispatch_get_current();
  ASSERT(curr_thread);
  return curr_thread->process->cwd;
}

long sys_mkdir(const char *path, mode_t mode) {
  if (!path)
    return -EINVAL;
  return vfs_mkdir(path, get_cwd(), mode);
}

long sys_rmdir(const char *path) {
  if (!path)
    return -EINVAL;
  return vfs_rmdir(path, get_cwd());
}
long sys_unlink(const char *path) {
  if (!path)
    return -EINVAL;
  return vfs_unlink(path, get_cwd());
}

long sys_rename(const char *oldpath, const char *newpath) {
  if (!oldpath || !newpath)
    return -EINVAL;
  return vfs_rename(oldpath, get_cwd(), newpath);
}

long sys_symlink(const char *target, const char *linkpath) {
  if (!target || !linkpath)
    return -EINVAL;
  return vfs_symlink(target, get_cwd(), linkpath);
}

long sys_readlink(const char *path, char *buf, size_t bufsize) {
  if (!path || !buf || bufsize == 0)
    return -EINVAL;
  return vfs_readlink(path, get_cwd(), buf, bufsize);
}
long sys_mount(const char *source, const char *target, const char *fs_name,
               unsigned long flags, void *data) {
  if (!target || !fs_name)
    return -EINVAL;
  return vfs_mount(source, target, get_cwd(), fs_name, flags, data);
}

long sys_umount(const char *target) {
  if (!target)
    return -EINVAL;
  return vfs_umount(target, get_cwd());
}

long sys_open(const char *pathname, int flags) {
  if (!pathname)
    return -EINVAL;

  int fd = vfs_open(pathname, get_cwd(), flags);
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

long sys_getdents(fd_t fd, vdir_entry_t *dentry, uint32_t count) {
  if (!dentry || count <= 0)
    return -EINVAL;

  return vfs_getdents(fd, dentry, count);
}

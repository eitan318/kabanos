#include "fs/vfs.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "klib/unistd.h"
#include "ksys/stat.h"

long sys_mkdir(const char *path, mode_t mode);
long sys_rmdir(const char *path);
long sys_unlink(const char *path);
long sys_rename(const char *oldpath, const char *newpath);
long sys_symlink(const char *target, const char *linkpath);
long sys_readlink(const char *path, char *buf, size_t bufsize);
long sys_mount(const char *source, const char *target, const char *fs_name,
               unsigned long flags, void *data);
long sys_umount(const char *target);
long sys_open(const char *pathname, int flags);
long sys_close(int fd);
long sys_lseek(int fd, off_t offset, int whence);
long sys_stat(int fd, fstat_t *user_stat);
long sys_read(int fd, char *buf, size_t count);
long sys_write(int fd, const char *buf, size_t len);
long sys_getdents(int fd, vdir_entry_t *dentry, int count);

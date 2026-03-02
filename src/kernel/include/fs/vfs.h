#pragma once
#include "fs/fs_common.h"
#include "ksys/stat.h"
#include "ksys/types.h"

#define DT_UNKNOWN 0
#define DT_REG 8 // Regular file
#define DT_DIR 4 // Directory
#define DT_LNK 10
#define MAX_SYMLINK_DEPTH 40

typedef unsigned mode_t;

// Only expose opaque pointers and syscall API
typedef struct super_block super_block_t;
typedef struct vnode vnode_t;
typedef struct fstat fstat_t;
typedef struct vdir_entry VDirEntry;

extern fs_platform_t g_vfs_platform;

struct fstat {
  uint32_t size;        // file/directory size in bytes
  int mode;             // file type
  uint32_t links_count; // hard link count
  uint32_t uid;         // owner user ID
  uint32_t gid;         // owner group ID
  uint32_t permissions; // file permissions
  uint64_t atime;       // access time
  uint64_t mtime;       // modification time
  uint64_t ctime;       // creation time
  int ino;
  const char *name;
};

struct vdir_entry {
  char file_name[32];
  uint32_t inode_num;
};

// FILE
int vfs_open(const char *path, int flags);
int vfs_close(int fd);
int vfs_rename(const char *oldpath, const char *newpath);
ssize_t vfs_read(int fd, void *buf, size_t size);
ssize_t vfs_write(int fd, const void *buf, size_t size);
int vfs_create(const char *path, mode_t mode);
int vfs_unlink(const char *path);
off_t vfs_seek(int fd, off_t offset, int whence);

int vfs_fstat(int fd, fstat_t *stat);

// DIR
int vfs_iter_dir(int fd, VDirEntry *dentry, int count);
int vfs_rmdir(const char *path);
int vfs_mkdir(const char *path, mode_t mode);

// SYMLINK
int vfs_symlink(const char *target, const char *linkpath);
ssize_t vfs_readlink(const char *path, char *buf, size_t bufsize);

// MOUNT
int vfs_mount(const char *source_dev, const char *target_path,
              const char *fs_name, unsigned long mountflags, void *fs_data);
int vfs_umount(const char *target);

int vfs_bind_vnode_to_fd(vnode_t *vnode, int flags);

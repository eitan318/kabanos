/**
 * @file vfs.h
 * @brief Virtual File System (VFS) Interface.
 * * Provides a unified abstraction layer over different physical filesystems,
 * supporting standard POSIX-like file and directory operations.
 */

#pragma once
#include "fs/fs_common.h"
#include "ksys/stat.h"
#include "ksys/types.h"

#define DT_UNKNOWN 0
#define DT_REG 8  /**< Regular file type */
#define DT_DIR 4  /**< Directory type */
#define DT_LNK 10 /**< Symbolic link type */
#define MAX_SYMLINK_DEPTH 40

typedef unsigned mode_t;

typedef struct super_block super_block_t;
typedef struct vnode vnode_t;

/** @brief VFS platform-specific hooks for memory and I/O. */
extern fs_platform_t g_vfs_platform;

/** @brief File status structure returned by vfs_fstat. */
typedef struct {
  uint32_t size;        /**< File size in bytes */
  int mode;             /**< File type and permissions */
  uint32_t links_count; /**< Number of hard links */
  uint32_t uid;         /**< Owner User ID */
  uint32_t gid;         /**< Owner Group ID */
  uint32_t permissions; /**< Permission bits */
  uint64_t atime;       /**< Last access time */
  uint64_t mtime;       /**< Last modification time */
  uint64_t ctime;       /**< Creation time */
  int ino;              /**< Inode number */
  const char *name;     /**< File name component */
} fstat_t;

/** @brief Directory entry structure for vfs_getdents. */
typedef struct {
  uint32_t inode_num; /**< Associated inode number */
  char file_name[32]; /**< Name of the directory entry */
} vdir_entry_t;

/* --- FILE OPERATIONS --- */

/** @brief Opens a file at the given path. Returns file descriptor or negative
 * error. */
int vfs_open(const char *path, vnode_t *base_node, int flags, int mode);

/** @brief Closes an open file descriptor. */
int vfs_close(int fd);

/** @brief Renames or moves a file from oldpath to newpath. */
int vfs_rename(const char *oldpath, vnode_t *cwd, const char *newpath);

/** @brief Reads up to size bytes into buf from the given fd. */
ssize_t vfs_read(int fd, void *buf, size_t size);

/** @brief Writes size bytes from buf to the given fd. */
ssize_t vfs_write(int fd, const void *buf, size_t size);

/** @brief Creates a new regular file. */
int vfs_create(const char *path, vnode_t *cwd, mode_t mode);

/** @brief Deletes a name from the filesystem. */
int vfs_unlink(const char *path, vnode_t *cwd);

/** @brief Changes the file offset of an open fd. */
off_t vfs_seek(int fd, off_t offset, int whence);

/** @brief Retrieves metadata for an open file descriptor. */
int vfs_fstat(int fd, fstat_t *stat);

/* --- DIRECTORY OPERATIONS --- */

/** @brief Removes an empty directory. */
int vfs_rmdir(const char *path, vnode_t *cwd);

/** @brief Creates a new directory. */
int vfs_mkdir(const char *path, vnode_t *cwd, mode_t mode);

/** @brief Reads directory entries from an open directory fd. */
int vfs_getdents(int fd, vdir_entry_t *dentry, uint32_t count);

/* --- SYMBOLIC LINK OPERATIONS --- */

/** @brief Creates a symbolic link pointing to target at linkpath. */
int vfs_symlink(const char *target, vnode_t *cwd, const char *linkpath);

/** @brief Reads the target of a symbolic link into buf. */
ssize_t vfs_readlink(const char *path, vnode_t *cwd, char *buf, size_t bufsize);

/* --- MOUNT OPERATIONS --- */

/** @brief Mounts a filesystem from source_dev onto target_path. */
int vfs_mount(const char *source_dev, const char *target_path, vnode_t *cwd,
              const char *fs_name, unsigned long mountflags, void *fs_data);

/** @brief Unmounts the filesystem at the given target path. */
int vfs_umount(const char *target, vnode_t *cwd);

/** @brief Internal helper to bind a vnode to a new file descriptor. */
int vfs_bind_vnode_to_fd(vnode_t *vnode, int flags);

/**
 * @file vfs_internal.h
 * @brief VFS core structures shared between the VFS and filesystem drivers.
 */
#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"
#include "drivers/block/blockdev.h"
#include "fs/vfs.h"

#define PATH_LEN_MAX 256

/* Forward declarations */
typedef struct mount_point mount_point_t;
typedef struct file file_t;
typedef struct fs_type fs_type_t;
typedef struct dir_ctx dir_ctx_t;

/** @brief Namespace operations a filesystem implements per vnode. */
struct vnode_ops {
  int (*create)(vnode_t *dir, const char *name, mode_t mode);
  int (*mkdir)(vnode_t *dir, const char *name, mode_t mode);
  int (*rmdir)(vnode_t *parent, const char *dir_name);
  int (*unlink)(vnode_t *dir, const char *name);
  void (*destroy)(vnode_t *vnode);
  vnode_t *(*lookup)(vnode_t *dir, const char *name);
  int (*symlink)(vnode_t *dir, const char *link_name, const char *target);
  int (*rename)(vnode_t *old_parent, const char *old_name, vnode_t *new_parent,
                const char *new_name);

  /** @brief Reads a symlink target into @p buf; returns its length. */
  ssize_t (*readlink)(vnode_t *vnode, char *buf, size_t bufsize);
};

/** @brief Operations on an open file. */
typedef struct {
  int (*open)(file_t *file);
  ssize_t (*read)(file_t *file, void *buf, size_t size);
  ssize_t (*write)(file_t *file, const void *buf, size_t size);
  int (*close)(file_t *file);
  off_t (*seek)(file_t *file, off_t offset);
  int (*fstat)(file_t *file, fstat_t *stat);
  int (*iter_dir)(file_t *parent_file, dir_ctx_t *ctx);
} file_ops_t;

/** @brief An open file: a vnode plus per-open state. */
struct file {
  vnode_t *vnode;
  file_ops_t *f_ops;
  off_t pos;    /**< Current file position. */
  int flags;    /**< O_RDONLY, O_WRONLY, etc. */
  int refcount; /**< Shared by dup'ed / inherited descriptors. */
  void *private_data;
};

/**
 * @brief Directory iteration context passed to iter_dir; the filesystem
 *        calls @ref dir_emit for every entry.
 */
struct dir_ctx {
  off_t *pos; /**< In/out directory offset. */
  void *buf;  /**< Destination buffer of the caller. */
  int (*actor)(dir_ctx_t *ctx, const char *name, int namelen, off_t offset,
               int ino, int type); /**< Callback invoked per entry. */
  int count;                       /**< Remaining capacity of @ref buf. */
};

/** @brief An active mount, linking a path to a mounted super block. */
struct mount_point {
  char path[256]; /**< Absolute mount path. */
  super_block_t *super_block;
  struct mount_point *next; /**< Next in the global mount list. */
};

/** @brief Per-mount filesystem instance. */
struct super_block {
  fs_type_t *fs_type;
  vnode_t *fs_root; /**< Root vnode of this filesystem. */
  file_ops_t *f_ops;
  struct vnode_ops *v_ops;
  vnode_t *vnode_cache; /**< Cached vnodes of this mount (linked list). */

  void *fs_private; /**< Filesystem-specific superblock data. */
};

/** @brief In-memory inode representation, shared across the VFS. */
struct vnode {
  super_block_t *super_block;
  void *fs_specific; /**< Filesystem-private inode data. */
  int size;
  int mode;          /**< File type and permission bits (S_IF*). */
  int device_handle; /**< Backing device handle for device nodes. */
  uint32_t i_ino;
  int refcount;       /**< Cache reference count. */
  struct vnode *next; /**< Next vnode in the superblock's cache list. */
};

/** @brief A registered filesystem driver. */
struct fs_type {
  const char *name; /**< e.g. "myfs". */
  int fs_flags;
  int (*fill_sb)(super_block_t *vfs_sb, blkdev_t *dev); /**< Mount hook. */
  int (*kill_sb)(super_block_t *sb);                    /**< Unmount hook. */
  struct fs_type *next;
};

/** @brief Feeds one directory entry to the iteration callback. */
static inline int dir_emit(dir_ctx_t *ctx, const char *name, int namelen,
                           off_t offset, ino_t ino, unsigned type) {
  if (ctx->actor) {
    return ctx->actor(ctx, name, namelen, offset, ino, type);
  }
  return 0;
}

/** @brief Finds the mount point whose path is the longest prefix of @p path. */
mount_point_t *vfs_find_mount_point(const char *path, vnode_t *const base_node);

int vfs_fs_type_register(fs_type_t *fs_type);
int vfs_fs_type_unregister(const char *name);

/**
 * @brief Resolves a path to a vnode, following intermediate symlinks.
 * @param follow_final_symlink Whether to also follow a symlink in the
 *        final component (false for lstat-like semantics).
 * @return Referenced vnode, or NULL if the path does not resolve.
 */
vnode_t *vfs_lookup_path(const char *path, vnode_t *cwd,
                         bool follow_final_symlink);

/** @brief Allocates a vnode and inserts it into the superblock cache. */
vnode_t *vfs_vnode_alloc(super_block_t *sb, ino_t ino, mode_t mode, size_t size,
                         void *fs_specific);

/** @brief Looks up a registered filesystem driver by name. */
fs_type_t *get_fs_type(const char *name);

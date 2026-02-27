#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"
//#include "klib/stdlib.h"
#include "device.h"
#include "vfs.h"

//#define PATH_SAPERATOR '/'
#define PATH_LEN_MAX 256

// Forward declarations
typedef struct mount_point mount_point_t;
typedef struct file file_t;
typedef struct fs_type fs_type_t;
typedef struct dir_ctx dir_ctx_t;

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

  // returns len of content
  ssize_t (*readlink)(vnode_t *vnode, char *buf, size_t bufsize);
};

struct file_ops {
  int (*open)(file_t *file);
  ssize_t (*read)(file_t *file, void *buf, size_t size);
  ssize_t (*write)(file_t *file, const void *buf, size_t size);
  int (*close)(file_t *file);
  off_t (*seek)(file_t *file, off_t offset);
  int (*iter_dir)(file_t *parent_file, dir_ctx_t *ctx);
};

// File structure - represents an open file
struct file {
  vnode_t *vnode; // Pointer to the vnode/inode
  struct file_ops *f_ops;
  off_t pos;    // Current file position
  int flags;    // O_RDONLY, O_WRONLY, etc
  int refcount; // For multiple opens of the same file
  void *private_data;
};

struct dir_ctx {
  off_t *pos;
  void *buf;
  int (*actor)(dir_ctx_t *ctx, const char *name, int namelen, off_t offset,
               int ino, int type);
  int count;
};

// Mount point structure
struct mount_point {
  char path[256]; // Mount path
  super_block_t *super_block;
  struct mount_point *next; // Linked list of mount points
};

struct super_block {
  fs_type_t *fs_type;
  vnode_t *fs_root;
  struct file_ops *f_ops;
  struct vnode_ops *v_ops;
  struct Vnode *vnode_cache;

  void *fs_private;
};

// Vnode structure - represents a file/directory in the VFS
struct vnode {
  super_block_t *super_block;
  void *fs_specific;
  int size;
  int mode;
  uint32_t i_ino;
  int refcount;       // For cache reference counting
  struct vnode *next; // Linked list for cache
};

struct fs_type {
  const char *name; // "ext4", "tmpfs", etc.
  int fs_flags;
  int (*fill_sb)(super_block_t *vfs_sb, blkdev_t *dev);
  int (*kill_sb)(super_block_t *sb);
  struct fs_type *next;
};

// Add this helper macro/function for directory emission
static inline int dir_emit(dir_ctx_t *ctx, const char *name, int namelen,
                           off_t offset, ino_t ino, unsigned type) {
  if (ctx->actor) {
    return ctx->actor(ctx, name, namelen, offset, ino, type);
  }
  return 0;
}

mount_point_t *vfs_find_mount_point(const char *path);
int vfs_fs_type_register(fs_type_t *fs_type);
int vfs_fs_type_unregister(const char *name);

vnode_t *vfs_lookup_path(const char *path, bool follow_final_symlink);

vnode_t *vfs_vnode_alloc(super_block_t *sb, ino_t ino, mode_t mode, size_t size,
                         void *fs_specific);
fs_type_t *get_fs_type(const char *name);

#define MAX_FD 256
static file_t *fd_table[MAX_FD] = {0};

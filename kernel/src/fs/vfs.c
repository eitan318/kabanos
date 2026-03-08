#include "fs/vfs.h"
#include "fs/fd.h"
#include "fs/fs_common.h"
#include "fs/vfs_internal.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "ksys/fcntl.h"
#include "mm/kmalloc.h"

mount_point_t *mount_p_table = NULL;
fs_type_t *fs_registry_table = NULL;

static int kernel_write_block(const void *dev, uint32_t lba, const void *buf) {
  blkdev_t *device = (blkdev_t *)dev;
  return device->write_sectors(device, lba, 4, buf);
}

static int kernel_read_block(const void *dev, uint32_t lba, void *buf) {
  blkdev_t *device = (blkdev_t *)dev;
  return device->read_sectors(device, lba, 4, buf);
}

fs_platform_t g_vfs_platform =
    (fs_platform_t){.alloc = kmalloc,
                    .free = kfree,
                    .read_block = kernel_read_block,
                    .write_block = kernel_write_block,
                    .log = kdebugf};

static int filldir(dir_ctx_t *ctx, const char *name, int namelen, off_t offset,
                   int ino, int type) {
  if (!ctx->buf || !name)
    return -1;

  // Get pointer to the current entry in the buffer
  vdir_entry_t *entries = (vdir_entry_t *)ctx->buf;
  int index = (int)(*ctx->pos); // Current position in buffer

  // Check if we have space (assuming ctx->count is the max)
  if (index >= ctx->count)
    return -1; // Buffer full

  entries[index].inode_num = ino;

  int copy_len = namelen < (sizeof(entries[index].file_name) - 1)
                     ? namelen
                     : (sizeof(entries[index].file_name) - 1);
  memcpy(entries[index].file_name, name, copy_len);
  entries[index].file_name[copy_len] = '\0';

  (*ctx->pos)++;
  return 0; // Success
}

int vfs_getdents(fd_t fd, vdir_entry_t *dentry, uint32_t count) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd] || !dentry)
    return -1;

  file_t *file = g_fd_table[fd];

  off_t entries_read = 0; // Track how many entries we've read

  dir_ctx_t ctx = {
      .actor = filldir,
      .pos = &entries_read,
      .buf = dentry,
      .count = count,
  };

  int result = file->f_ops->iter_dir(file, &ctx);

  if (result < 0)
    return result;

  return (int)entries_read;
}

int vfs_fs_type_register(fs_type_t *fs_type) {
  if (!fs_type)
    return -1;

  fs_type_t *curr = fs_registry_table;
  while (curr) {
    if (strcmp(curr->name, fs_type->name) == 0) {
      return -1;
    }
    curr = curr->next;
  }

  fs_type->next = fs_registry_table;
  fs_registry_table = fs_type;
  return 0;
}

int vfs_fs_type_unregister(const char *name) {
  if (!name || !fs_registry_table)
    return -1;

  fs_type_t *prev = NULL;
  fs_type_t *curr = fs_registry_table;

  while (curr) {
    if (strcmp(curr->name, name) == 0) {
      // found match → unlink
      if (prev) {
        prev->next = curr->next;
      } else {
        // removing head
        fs_registry_table = curr->next;
      }
      curr->next = NULL; // safety
      return 0;          // success
    }
    prev = curr;
    curr = curr->next;
  }

  return -1; // not found
}

vnode_t *vfs_vnode_alloc(super_block_t *sb, ino_t ino, mode_t mode, size_t size,
                         void *fs_specific) {
  vnode_t *new_vnode = kmalloc(sizeof(vnode_t));
  if (!new_vnode) {
    return NULL;
  }

  // FIXED: Don't store inode pointer in fs_specific
  new_vnode->fs_specific = fs_specific;
  new_vnode->i_ino = ino;
  new_vnode->size = size;
  new_vnode->mode = mode;
  new_vnode->refcount = 1; // VFS refcount, not inode refcount
  new_vnode->super_block = sb;

  // Don't call myfs_iput here - let caller manage the inode
  return new_vnode;
}

void vfs_vnode_kfree(vnode_t *v) {}

static void vnode_get(vnode_t *vnode) {
  if (vnode)
    vnode->refcount++;
}

static void vnode_put(vnode_t *vnode) {
  if (!vnode)
    return;
  vnode->refcount--;
  if (vnode->refcount == 0) {
    // Remove from cache list
    vnode_t **prev = &(vnode->super_block->vnode_cache);
    while (*prev && *prev != vnode)
      prev = &(*prev)->next;
    if (*prev == vnode)
      *prev = vnode->next;
    if (vnode && vnode->super_block && vnode->super_block->v_ops->destroy) {
      vnode->super_block->v_ops->destroy(vnode);
    }
    kfree(vnode);
  }
}

// Allocate a new file descriptor
static int alloc_fd(file_t *file) {
  for (int i = 0; i < MAX_FD; i++) {
    if (!g_fd_table[i]) {
      g_fd_table[i] = file;
      return i;
    }
  }
  return -1;
}

// Close a file descriptor
static void free_fd(int fd) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return;
  file_t *file = g_fd_table[fd];
  g_fd_table[fd] = NULL;
  file->refcount--;
  if (file->refcount == 0) {
    vnode_put(file->vnode);
    kfree(file);
  }
}

fs_type_t *get_fs_type(const char *name) {
  fs_type_t *fs = fs_registry_table;
  while (fs) {
    if (strcmp(fs->name, name) == 0) {
      return fs;
    }
    fs = fs->next;
  }
  return NULL;
}

int vfs_mount(const char *source_dev, const char *target_path,
              const char *fs_name, unsigned long mountflags, void *fs_data) {

  blkdev_t *dev = blkdev_get(source_dev);
  if (!dev) {
    kdebugf("vfs_mount: device %s not found\n", source_dev);
    return -ENODEV;
  }

  fs_type_t *fs_type = get_fs_type(fs_name);
  if (!fs_type) {
    return -ENODEV;
  }

  mount_point_t *mount_p = kmalloc(sizeof(*mount_p));
  if (!mount_p) {
    kfree(mount_p);
    return -ENOMEM;
  }
  super_block_t *super_block = kmalloc(sizeof(*super_block));
  if (!super_block) {
    kfree(mount_p);
    kfree(super_block);
    return -ENOMEM;
  }
  memset(super_block, 0, sizeof(*super_block));

  if (fs_type->fill_sb(super_block, dev) == -1) {
    kfree(mount_p);
    kfree(super_block);
    return -EIO;
  }

  /* 5. Finalize Mount Point */
  strncpy(mount_p->path, target_path, sizeof(mount_p->path) - 1);
  mount_p->path[sizeof(mount_p->path) - 1] = '\0';
  mount_p->super_block = super_block;

  // Add to mount table
  mount_p->next = mount_p_table;
  mount_p_table = mount_p;

  return 0;
}

int vfs_symlink(const char *target, const char *linkpath) {
  // Parse linkpath into parent + name
  char *path_copy = strdup(linkpath);
  char *last_slash = strrchr(path_copy, '/');
  *last_slash = '\0';
  char *link_name = last_slash + 1;
  char *parent_path = (*path_copy) ? path_copy : "/";

  vnode_t *parent = vfs_lookup_path(parent_path, true);
  if (!parent) {
    kfree(path_copy);
    return -1;
  }

  int result = parent->super_block->v_ops->symlink(parent, link_name, target);

  vnode_put(parent);
  kfree(path_copy);
  return result;
}

ssize_t vfs_readlink(const char *path, char *buf, size_t bufsize) {
  vnode_t *vnode =
      vfs_lookup_path(path, false); // false = don't follow symlinks

  if (!vnode) {
    return -1;
  }

  if (!S_ISLNK(vnode->mode)) {
    vnode_put(vnode);
    return -1;
  }

  if (!vnode->super_block->v_ops->readlink) {
    vnode_put(vnode);
    return -1;
  }

  ssize_t result = vnode->super_block->v_ops->readlink(vnode, buf, bufsize);
  vnode_put(vnode);
  return result;
}

static int vfs_remove_mount_point(const char *path) {
  mount_point_t *prev = NULL;
  mount_point_t *curr = mount_p_table;

  while (curr) {
    if (strcmp(curr->path, path) == 0) {
      // Unlink from list
      if (prev) {
        prev->next = curr->next;
      } else {
        mount_p_table = curr->next;
      }

      kfree(curr);
      return 0;
    }

    prev = curr;
    curr = curr->next;
  }

  return -1;
}

int vfs_rename(const char *oldpath, const char *newpath) {
  // Parse old path
  char *old_copy = strdup(oldpath);
  char *old_slash = strrchr(old_copy, '/');
  if (!old_slash) {
    kfree(old_copy);
    return -1;
  }
  *old_slash = '\0';
  const char *old_name = old_slash + 1;
  const char *old_parent_path = (*old_copy) ? old_copy : "/";

  // Parse new path
  char *new_copy = strdup(newpath);
  char *new_slash = strrchr(new_copy, '/');
  if (!new_slash) {
    kfree(old_copy);
    kfree(new_copy);
    return -1;
  }
  *new_slash = '\0';
  const char *new_name = new_slash + 1;
  const char *new_parent_path = (*new_copy) ? new_copy : "/";

  // Lookup parent directories
  vnode_t *old_parent = vfs_lookup_path(old_parent_path, true);
  vnode_t *new_parent = vfs_lookup_path(new_parent_path, true);

  if (!old_parent || !new_parent) {
    if (old_parent)
      vnode_put(old_parent);
    if (new_parent)
      vnode_put(new_parent);
    kfree(old_copy);
    kfree(new_copy);
    return -1;
  }

  // Check if both are on the same filesystem
  if (old_parent->super_block != new_parent->super_block) {
    // Cross-filesystem move not supported directly
    vnode_put(old_parent);
    vnode_put(new_parent);
    kfree(old_copy);
    kfree(new_copy);
    return -1; // EXDEV error
  }

  // Call filesystem-specific rename
  int result = -1;
  if (old_parent->super_block->v_ops->rename) {
    result = old_parent->super_block->v_ops->rename(old_parent, old_name,
                                                    new_parent, new_name);
  }

  vnode_put(old_parent);
  vnode_put(new_parent);
  kfree(old_copy);
  kfree(new_copy);
  return result;
}

int vfs_umount(const char *target) {

  mount_point_t *mp = vfs_find_mount_point(target);
  super_block_t *sb = mp->super_block;
  sb->fs_type->kill_sb(sb);
  vfs_remove_mount_point(target);
  return 0;
}

// Find the best matching mount point for a path
mount_point_t *vfs_find_mount_point(const char *path) {
  mount_point_t *best_match = NULL;
  int best_match_len = 0;

  // Find the longest matching mount path
  for (mount_point_t *mount = mount_p_table; mount; mount = mount->next) {
    int mount_len = strlen(mount->path);

    // Check if this mount path is a prefix of the target path
    if (strncmp(path, mount->path, mount_len) == 0 &&
        (path[mount_len] == '/' || path[mount_len] == '\0' ||
         strcmp(mount->path, "/") == 0)) {

      if (mount_len > best_match_len) {
        best_match = mount;
        best_match_len = mount_len;
      }
    }
  }

  return best_match;
}

vnode_t *vfs_lookup_path(const char *path, bool follow_final_symlink) {
  if (!path || path[0] != '/') {
    return NULL;
  }

  mount_point_t *mount = vfs_find_mount_point(path);
  if (!mount) {
    return NULL;
  }

  const char *relative_path = path + strlen(mount->path);
  if (relative_path[0] == '/') {
    relative_path++;
  }

  vnode_t *current = mount->super_block->fs_root;
  if (!current) {
    return NULL;
  }

  vnode_get(current);

  if (strlen(relative_path) == 0) {
    return current;
  }

  char *path_copy = strdup(relative_path);
  if (!path_copy) {
    vnode_put(current);
    return NULL;
  }

  char *saveptr;
  char *token = strtok_r(path_copy, "/", &saveptr);
  int symlink_depth = 0;

  while (token != NULL) {
    if (!current->super_block->v_ops || !current->super_block->v_ops->lookup) {
      vnode_put(current);
      kfree(path_copy);
      return NULL;
    }

    vnode_t *next = current->super_block->v_ops->lookup(current, token);
    if (!next) {
      vnode_put(current);
      kfree(path_copy);
      return NULL;
    }

    vnode_get(next);

    // --- Check for symlink ---
    bool is_final = (saveptr == NULL || *saveptr == '\0');
    if (S_ISLNK(next->mode)) {
      // If final component and not following symlinks, return it
      if (is_final && !follow_final_symlink) {
        vnode_put(current);
        kfree(path_copy);
        return next;
      }

      if (++symlink_depth > MAX_SYMLINK_DEPTH) {
        vnode_put(next);
        vnode_put(current);
        kfree(path_copy);
        return NULL;
      }

      char target[PATH_LEN_MAX];
      ssize_t len =
          next->super_block->v_ops->readlink(next, target, sizeof(target) - 1);
      if (len < 0) {
        vnode_put(next);
        vnode_put(current);
        kfree(path_copy);
        return NULL;
      }
      target[len] = '\0';

      vnode_put(next);

      if (target[0] == '/') {
        // Absolute symlink → restart from root
        vnode_put(current);
        kfree(path_copy);
        return vfs_lookup_path(target, follow_final_symlink);
      } else {
        // Relative symlink → build new path: target + remaining
        char *remaining = strtok_r(NULL, "/", &saveptr);
        char new_relative_path[PATH_LEN_MAX];

        if (remaining) {
          ksnprintf(new_relative_path, sizeof(new_relative_path), "%s/%s",
                    target, remaining);
        } else {
          strncpy(new_relative_path, target, sizeof(new_relative_path) - 1);
          new_relative_path[sizeof(new_relative_path) - 1] = '\0';
        }

        kfree(path_copy);
        path_copy = strdup(new_relative_path);
        if (!path_copy) {
          vnode_put(current);
          return NULL;
        }

        token = strtok_r(path_copy, "/", &saveptr);
        continue;
      }
    }

    // --- Normal case: descend ---
    vnode_put(current);
    current = next;
    token = strtok_r(NULL, "/", &saveptr);
  }

  kfree(path_copy);
  return current;
}

int vfs_bind_vnode_to_fd(vnode_t *vnode, int flags) {
  file_t *file = kmalloc(sizeof(file_t));
  if (!file)
    return -1;

  file->vnode = vnode;
  file->pos = 0;
  file->flags = flags;
  file->refcount = 1;

  file->f_ops = vnode->super_block->f_ops;

  if (file->f_ops && file->f_ops->open) {
    if (file->f_ops->open(file) < 0) {
      kfree(file);
      return -1;
    }
  }

  // 3. Put it in the table
  return alloc_fd(file);
}

int vfs_open(const char *path, int flags) {
  vnode_t *vnode = vfs_lookup_path(path, true);
  if (!vnode)
    return -1;

  return vfs_bind_vnode_to_fd(vnode, flags);
}

off_t vfs_seek(int fd, off_t relative_offset, int whence) {
  file_t *file = g_fd_table[fd];
  off_t whence_off = 0;
  switch (whence) {
  case SEEK_SET:
    whence_off = 0;
    break;
  case SEEK_CUR:
    whence_off = file->pos;
    break;
  case SEEK_END:
    whence_off = file->vnode->size;
    break;
  default:
    return -1;
  }
  off_t new_pos = whence_off + relative_offset;

  if (new_pos < 0 || new_pos > file->vnode->size)
    return -1; // EINVAL or beyond EOF
  file->pos = new_pos;
  return new_pos;
}

ssize_t vfs_read(int fd, void *buf, size_t size) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -1;

  file_t *file = g_fd_table[fd];

  // Use file operations if available, otherwise use vnode operations
  int n = 0;
  if (file->f_ops && file->f_ops->read) {
    n = file->f_ops->read(file, buf, size);
  } else {
    return -1; // No read operation available
  }

  if (n > 0)
    file->pos += n;

  return n;
}

int vfs_fstat(int fd, fstat_t *stat) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -1;

  file_t *file = g_fd_table[fd];

  int n = 0;
  if (file->f_ops && file->f_ops->fstat) {
    n = file->f_ops->fstat(file, stat);
  } else {
    return -1;
  }
  return n;
}

// return bytes written
ssize_t vfs_write(int fd, const void *buf, size_t size) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -1;
  file_t *file = g_fd_table[fd];

  // Check write permissions

  int mode = file->flags & O_ACCMODE;
  if (mode != O_WRONLY && mode != O_RDWR) {
    return -1; // not writable
  }

  // Use file operations if available, otherwise use vnode operations
  int n = 0;
  if (file->f_ops && file->f_ops->write) {
    n = file->f_ops->write(file, buf, size);
  } else {
    return -1; // No write operation available
  }

  if (n > 0)
    file->pos += n;

  return n;
}

// Close a file descriptor
int vfs_close(int fd) {
  if (fd < 0 || fd >= MAX_FD || !g_fd_table[fd])
    return -1;

  file_t *file = g_fd_table[fd];

  // Call close operation if available
  int result = 0;
  if (file->f_ops && file->f_ops->close) {
    result = file->f_ops->close(file);
  }

  free_fd(fd);
  return result;
}

// Common helper for operations that need parent dir + child name
static int vfs_parent_operation(const char *path,
                                int (*op)(vnode_t *parent, const char *name,
                                          mode_t mode),
                                mode_t mode) {
  if (!path || !op) {
    return -1;
  }

  char *path_copy = strdup(path);
  if (!path_copy)
    return -1;

  char *last_slash = strrchr(path_copy, '/');
  if (!last_slash) {
    kfree(path_copy);
    return -1;
  }

  *last_slash = '\0';
  char *child_name = last_slash + 1;
  char *parent_path = (*path_copy) ? path_copy : "/";

  vnode_t *parent = vfs_lookup_path(parent_path, true);
  if (!parent) {
    kfree(path_copy);
    return -1;
  }

  int result = op(parent, child_name, mode);

  vnode_put(parent);
  kfree(path_copy);
  return result;
}

int vfs_create(const char *path, mode_t mode) {

  mount_point_t *mp = vfs_find_mount_point(path);
  struct vnode_ops *vnode_ops = mp->super_block->v_ops;

  if (!mp || !vnode_ops || !vnode_ops->create) {
    return -1;
  }
  return vfs_parent_operation(path, vnode_ops->create, mode);
}

int vfs_mkdir(const char *path, mode_t mode) {
  mount_point_t *mp = vfs_find_mount_point(path);
  struct vnode_ops *vnode_ops = mp->super_block->v_ops;
  if (!mp || !vnode_ops || !vnode_ops->mkdir) {
    return -1;
  }
  return vfs_parent_operation(path, vnode_ops->mkdir, mode);
}

int vfs_rmdir(const char *path) {
  mount_point_t *mp = vfs_find_mount_point(path);
  struct vnode_ops *vnode_ops = mp->super_block->v_ops;
  if (!mp || !vnode_ops || !vnode_ops->mkdir) {
    return -1;
  }
  return vfs_parent_operation(
      path, (int (*)(vnode_t *, const char *, mode_t))vnode_ops->rmdir, 0);
}

int vfs_unlink(const char *path) {
  mount_point_t *mp = vfs_find_mount_point(path);
  struct vnode_ops *vnode_ops = mp->super_block->v_ops;
  if (!mp || !vnode_ops || !vnode_ops->unlink) {
    return -1;
  }
  // unlink doesn't use mode, just pass 0
  return vfs_parent_operation(
      path, (int (*)(vnode_t *, const char *, mode_t))vnode_ops->unlink, 0);
}

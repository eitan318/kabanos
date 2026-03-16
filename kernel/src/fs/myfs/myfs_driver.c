#include "drivers/block/blockdev.h"
#include "fs/myfs/myfs.h"
#include "fs/vfs.h"
#include "fs/vfs_internal.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "klib/errno.h"
#include "ksys/stat.h"
#include "ksys/stat.h"
#include "mm/kmalloc.h"
#include "modules.h"

static void myfsd_v_destroy(vnode_t *vnode) {}

static int myfsd_f_dir_iter(file_t *parent_file, dir_ctx_t *ctx) {
  MyfsSuperBlock *myfs_sb = parent_file->vnode->super_block->fs_private;
  MyfsInode *parent = myfs_iget(myfs_sb, parent_file->vnode->i_ino);

  if (!parent || !S_ISDIR(parent->mode)) {
    if (parent)
      myfs_iput(myfs_sb, parent);
    return -1;
  }

  int entries_count = parent->size / sizeof(MyfsDirEntry);
  MyfsDirEntry *entries = kmalloc(parent->size);
  if (!entries) {
    myfs_iput(myfs_sb, parent);
    return -1;
  }

  int bytes_read = myfs_node_read(myfs_sb, parent, 0, entries, parent->size);
  if (bytes_read < 0) {
    kfree(entries);
    myfs_iput(myfs_sb, parent);
    return -1;
  }

  int actual_entries = bytes_read / sizeof(MyfsDirEntry);
  int emitted = 0;

  uint64_t start_index = parent_file->pos / sizeof(MyfsDirEntry);

  for (int i = start_index; i < actual_entries && emitted < ctx->count; i++) {
    MyfsDirEntry *entry = &entries[i];
    int namelen = strlen(entry->file_name);

    MyfsInode *child = myfs_iget(myfs_sb, entry->inode_num);
    unsigned type = S_ISDIR(child->mode) ? DT_DIR : DT_REG;
    myfs_iput(myfs_sb, child);

    if (dir_emit(ctx, entry->file_name, namelen, i * sizeof(MyfsDirEntry),
                 entry->inode_num, type) == 0) {
      emitted++;
      parent_file->pos += sizeof(MyfsDirEntry);
    }
  }

  kfree(entries);
  myfs_iput(myfs_sb, parent);
  return emitted;
}

static vnode_t *myfsd_v_lookup(vnode_t *dir, const char *name) {
  if (!S_ISDIR(dir->mode)) {
    return NULL;
  }

  MyfsSuperBlock *myfs_sb = dir->super_block->fs_private;

  MyfsInode *dir_inode = myfs_iget(myfs_sb, dir->i_ino);
  if (!dir_inode) {
    return NULL;
  }

  uint32_t child_ino;
  if (myfs_lookup(myfs_sb, dir_inode, name, &child_ino) < 0) {
    myfs_iput(myfs_sb, dir_inode);
    return NULL;
  }

  // Get child inode to extract metadata
  MyfsInode *child_inode = myfs_iget(myfs_sb, child_ino);
  if (!child_inode) {
    myfs_iput(myfs_sb, dir_inode);
    return NULL;
  }

  // Create VFS vnode from MyFS inode
  vnode_t *child_vnode = kmalloc(sizeof(vnode_t));
  if (!child_vnode) {
    myfs_iput(myfs_sb, child_inode);
    myfs_iput(myfs_sb, dir_inode);
    return NULL;
  }

  // Fill VFS vnode with MyFS inode data
  child_vnode->fs_specific = NULL;
  child_vnode->refcount = 1;
  child_vnode->size = child_inode->size;
  child_vnode->i_ino = child_inode->i_ino;
  child_vnode->mode = child_inode->mode;
  child_vnode->super_block = dir->super_block;

  // Release MyFS inode references (now balanced)
  myfs_iput(myfs_sb, child_inode);
  myfs_iput(myfs_sb, dir_inode);

  return child_vnode;
}

static int myfsd_v_create(vnode_t *dir, const char *name, mode_t mode) {
  if (!S_ISDIR(dir->mode)) {
    return -1;
  }

  MyfsSuperBlock *myfs_sb = dir->super_block->fs_private;

  MyfsInode *dir_inode = myfs_iget(myfs_sb, dir->i_ino);
  if (!dir_inode) {
    return -1;
  }

  uint32_t new_ino;
  int result = myfs_create_file(myfs_sb, dir_inode, name, &new_ino);

  // Update VFS directory vnode size
  if (result == 0) {
    dir->size = dir_inode->size;
  }

  myfs_iput(myfs_sb, dir_inode);
  return result;
}

static int myfsd_v_mkdir(vnode_t *dir, const char *name, mode_t mode) {
  if (!S_ISDIR(dir->mode)) {
    return -1;
  }

  MyfsSuperBlock *myfs_sb = dir->super_block->fs_private;

  MyfsInode *dir_inode = myfs_iget(myfs_sb, dir->i_ino);
  if (!dir_inode) {
    return -1;
  }

  uint32_t new_ino;
  int result = myfs_create_dir(myfs_sb, dir_inode, name, &new_ino);

  // Update VFS directory vnode size
  if (result == 0) {
    dir->size = dir_inode->size;
  }

  myfs_iput(myfs_sb, dir_inode);
  return result;
}

static int myfsd_v_unlink(vnode_t *dir, const char *name) {
  if (!S_ISDIR(dir->mode)) {
    return -1;
  }

  MyfsSuperBlock *myfs_sb = dir->super_block->fs_private;

  // FIXED: Get directory inode properly
  MyfsInode *dir_inode = myfs_iget(myfs_sb, dir->i_ino);
  if (!dir_inode) {
    return -1;
  }

  int result = myfs_unlink(myfs_sb, dir_inode, name);

  // Update VFS directory vnode size
  if (result == 0) {
    dir->size = dir_inode->size;
  }

  myfs_iput(myfs_sb, dir_inode);
  return result;
}

static int myfsd_getattr(MyfsInode *inode, fstat_t *stats) {
  stats->size = inode->size;
  stats->mode = inode->mode;
  stats->atime = inode->atime;
  stats->ctime = inode->ctime;
  stats->gid = inode->gid;
  stats->links_count = inode->links_count;
  stats->mtime = inode->mtime;
  stats->uid = inode->uid;
  stats->permissions = inode->permissions;
  stats->ino = inode->i_ino;
  return 0;
}

int myfsd_f_fstat(file_t *file, fstat_t *stat) {
  vnode_t *vnode = file->vnode;
  MyfsSuperBlock *myfs_sb = vnode->super_block->fs_private;

  MyfsInode *inode = myfs_iget(myfs_sb, vnode->i_ino);
  if (!inode) {
    return -1;
  }

  return myfsd_getattr(inode, stat);
}

static ssize_t myfsd_f_read(file_t *file, void *buf, size_t size) {
  vnode_t *vnode = file->vnode;
  MyfsSuperBlock *myfs_sb = vnode->super_block->fs_private;

  MyfsInode *inode = myfs_iget(myfs_sb, vnode->i_ino);
  if (!inode) {
    return -1;
  }

  ssize_t bytes_read = myfs_node_read(myfs_sb, inode, file->pos, buf, size);
  if (bytes_read > 0)
    file->pos += bytes_read;

  myfs_iput(myfs_sb, inode);
  return bytes_read;
}

static ssize_t myfsd_f_write(file_t *file, const void *buf, size_t size) {
  vnode_t *vnode = file->vnode;
  MyfsSuperBlock *myfs_sb = vnode->super_block->fs_private;

  MyfsInode *inode = myfs_iget(myfs_sb, vnode->i_ino);
  if (!inode) {
    return -1;
  }

  ssize_t result = myfs_node_write(myfs_sb, inode, file->pos, buf, size);
  if (result > 0) {
    file->pos += result; 
    vnode->size = inode->size;
  }

  myfs_iput(myfs_sb, inode);
  return result;
}

static int myfsd_f_open(file_t *file) {
  MyfsSuperBlock *myfs_sb = file->vnode->super_block->fs_private;
  MyfsInode *inode = myfs_iget(myfs_sb, file->vnode->i_ino);
  if (!inode) {
    return -1;
  }
  myfs_iput(file->vnode->super_block->fs_private, inode); // Release reference
  return 0;
}

static int myfsd_f_close(file_t *file) {
  vnode_t *vnode = file->vnode;
  myfs_disk_inode_write(vnode->super_block->fs_private, vnode->i_ino);

  return 0;
}

static int myfsd_v_rmdir(vnode_t *parent, const char *dir_name) {
  MyfsSuperBlock *myfs_sb = parent->super_block->fs_private;

  MyfsInode *dir_inode = myfs_iget(myfs_sb, parent->i_ino);
  if (!dir_inode)
    return -1;

  int result = myfs_remove_dir(myfs_sb, dir_inode, dir_name);

  if (result == 0)
    parent->size = dir_inode->size;

  myfs_iput(myfs_sb, dir_inode);
  return result;
}

static int myfsd_v_symlink(vnode_t *dir, const char *name, const char *target) {
  MyfsSuperBlock *myfs_sb = dir->super_block->fs_private;
  MyfsInode *dir_inode = myfs_iget(myfs_sb, dir->i_ino);

  int result = myfs_create_symlink(myfs_sb, dir_inode, name, target);

  myfs_iput(myfs_sb, dir_inode);
  return result;
}

static ssize_t myfsd_v_readlink(vnode_t *vnode, char *buf, size_t bufsize) {
  MyfsSuperBlock *myfs_sb = vnode->super_block->fs_private;
  MyfsInode *inode = myfs_iget(myfs_sb, vnode->i_ino);

  ssize_t result = myfs_symlink_read(myfs_sb, inode, buf, bufsize);

  myfs_iput(myfs_sb, inode);
  return result;
}

static int myfsd_v_rename(vnode_t *old_parent, const char *old_name,
                          vnode_t *new_parent, const char *new_name) {
  MyfsSuperBlock *myfs_sb = old_parent->super_block->fs_private;

  MyfsInode *old_parent_inode = myfs_iget(myfs_sb, old_parent->i_ino);
  MyfsInode *new_parent_inode = myfs_iget(myfs_sb, new_parent->i_ino);

  myfs_rename(myfs_sb, old_parent_inode, old_name, new_parent_inode, new_name);
  old_parent->size = old_parent_inode->size;
  new_parent->size = new_parent_inode->size;
  return 0;
}

static int myfsd_kill_vsb(super_block_t *vfs_sb) {
  if (vfs_sb && vfs_sb->fs_private) {
    MyfsSuperBlock *myfs_sb = (MyfsSuperBlock *)vfs_sb->fs_private;
    myfs_sb_kill(myfs_sb);
    vfs_sb->fs_private = NULL;
  }
  kfree(vfs_sb);
  return 0;
}

int myfsd_unregister(void) {
  vfs_fs_type_unregister(MYFS_NAME);
  return 0;
}

file_ops_t myfs_f_ops = {
    .open = myfsd_f_open,
    .read = myfsd_f_read,
    .write = myfsd_f_write,
    .close = myfsd_f_close,
    .iter_dir = myfsd_f_dir_iter,
    .fstat = myfsd_f_fstat,
};

struct vnode_ops myfs_vnode_ops = {
    .mkdir = myfsd_v_mkdir,
    .rmdir = myfsd_v_rmdir,
    .create = myfsd_v_create,
    .lookup = myfsd_v_lookup,
    .unlink = myfsd_v_unlink,
    .destroy = myfsd_v_destroy,
    .readlink = myfsd_v_readlink,
    .rename = myfsd_v_rename,
    .symlink = myfsd_v_symlink,
};

static int myfsd_super_sb_fill(super_block_t *vfs_sb, blkdev_t *dev) {
  // Mount the MyFS filesystem
  MyfsSuperBlock *myfs_sb = myfs_sb_read(dev);
  if (myfs_sb == NULL) {
    return -1;
  }

  // Configure VFS superblock
  vfs_sb->fs_private = myfs_sb; // Store MyFS superblock
  vfs_sb->fs_type = get_fs_type(MYFS_NAME);
  vfs_sb->f_ops = &myfs_f_ops;     // file_t operations
  vfs_sb->v_ops = &myfs_vnode_ops; // vnode_t operations

  MyfsInode *root_inode;
  root_inode = myfs_iget(myfs_sb, MYFS_ROOT_INODE_NUM);
  if (root_inode == NULL) {
    myfs_inode_alloc(myfs_sb, &root_inode, S_IFDIR);
  }

  myfs_dir_add_entry(myfs_sb, root_inode, ".", root_inode->i_ino);

  vnode_t *root_vnode = vfs_vnode_alloc(
      vfs_sb, root_inode->i_ino, root_inode->mode, root_inode->size, NULL);
  if (!root_vnode) {
    myfs_sb_kill(myfs_sb);
    return -1;
  }

  vfs_sb->fs_root = root_vnode;

  return 0;
}

int myfs_init(module_t *module) {
  fs_type_t *myfs_type;
  myfs_type = kmalloc(sizeof(*myfs_type));
  myfs_type->next = NULL;
  myfs_type->fill_sb = myfsd_super_sb_fill;
  myfs_type->fs_flags = 0;
  myfs_type->name = MYFS_NAME;
  myfs_type->kill_sb = myfsd_kill_vsb;

  if (vfs_fs_type_register(myfs_type) < 0) {
    return -1;
  }
  return 0;
}

static const char *myfs_deps[] = {"ata", NULL};

ITER_MODULE(myfs) = {
    .name = "myfs",
    .required_modules_names = myfs_deps,
    .init = &myfs_init,
    .fini = NULL,
};

#include "fat.h"
#include "mm/kmalloc.h"
#include "vfs_internal.h"

// Adapter: bridges file_t → FAT_File
static int fat_vfs_open(file_t *file) {
  // We need the path — store it in vnode->fs_specific during lookup
  const char *path = (const char *)file->vnode->fs_specific;
  FAT_File *ff = fat_open(path);
  if (!ff)
    return -1;
  file->vnode->fs_specific = ff; // replace path with open handle
  return 0;
}

static ssize_t fat_vfs_read(file_t *file, void *buf, size_t size) {
  FAT_File *ff = (FAT_File *)file->vnode->fs_specific;
  if (!ff)
    return -1;
  return (ssize_t)fat_read(ff, (uint32_t)size, buf);
}

static int fat_vfs_close(file_t *file) {
  FAT_File *ff = (FAT_File *)file->vnode->fs_specific;
  if (ff) {
    fat_close(ff);
    file->vnode->fs_specific = NULL;
  }
  return 0;
}

static int fat_iter_dir(file_t *file, dir_ctx_t *ctx) {
  FAT_File *ff = (FAT_File *)file->vnode->fs_specific;
  if (!ff)
    return -1;

  FAT_DirectoryEntry entry;
  int count = 0;

  while (fat_read_entry(ff, &entry)) {
    if (entry.name[0] == 0x00)
      break;
    if (entry.name[0] == 0xE5)
      continue; // deleted
    if (entry.attributes == FAT_ATTRIBUTE_LFN)
      continue;

    char name[13];
    int pos = 0;
    for (int i = 0; i < 8 && entry.name[i] != ' '; i++)
      name[pos++] = entry.name[i];
    if (entry.name[8] != ' ') {
      name[pos++] = '.';
      for (int i = 8; i < 11 && entry.name[i] != ' '; i++)
        name[pos++] = entry.name[i];
    }
    name[pos] = '\0';

    int type = (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    if (dir_emit(ctx, name, pos, *ctx->pos, entry.first_cluster_low, type) < 0)
      break;
    count++;
  }
  return count;
}

// Lookup: find a child entry by name, return a new vnode
static Vnode *fat_vfs_lookup(Vnode *dir, const char *name) {
  FAT_File *parent_ff = (FAT_File *)dir->fs_specific;
  if (!parent_ff)
    return NULL;

  FAT_DirectoryEntry entry;
  if (!fat_find_file(parent_ff, name, &entry))
    return NULL;

  FAT_File *child_ff = fat_open_entry(&entry);
  if (!child_ff)
    return NULL;

  mode_t mode = (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) ? (S_IFDIR | 0755)
                                                             : (S_IFREG | 0644);

  Vnode *vnode = vfs_vnode_alloc(dir->super_block, entry.first_cluster_low,
                                 mode, entry.size, child_ff);
  return vnode;
}

static struct file_ops fat_file_ops = {
    .open = fat_vfs_open,
    .read = fat_vfs_read,
    .close = fat_vfs_close,
    .iter_dir = fat_iter_dir,
};

static struct vnode_ops fat_vnode_ops = {
    .lookup = fat_vfs_lookup,
    // create/mkdir/unlink = NULL for read-only FAT
};

static int fat_fill_super(SuperBlock *sb) {
  sb->f_ops = &fat_file_ops;
  sb->v_ops = &fat_vnode_ops;
  sb->fs_private = NULL;

  // Get the root FAT_File* from fat — add fat_get_root() to fat.h
  FAT_File *root_ff = fat_get_root();

  Vnode *root = vfs_vnode_alloc(sb, 0, S_IFDIR | 0755, 0, root_ff);
  if (!root)
    return -1;

  sb->fs_root = root;
  return 0;
}

static int fat_kill_sb(SuperBlock *sb) {
  // free fat_table, root buffer, etc.
  if (g_fat_data.fat_table) {
    kfree(g_fat_data.fat_table);
    g_fat_data.fat_table = NULL;
  }
  if (g_fat_data.root_directory.buffer) {
    kfree(g_fat_data.root_directory.buffer);
    g_fat_data.root_directory.buffer = NULL;
  }
  g_fat_data.initialized = false;
  return 0;
}

static fs_type_t fat_fs_type = {
    .name = "FAT12",
    .fill_super_sb = fat_fill_super,
    .kill_sb = fat_kill_sb,
    .next = NULL,
};

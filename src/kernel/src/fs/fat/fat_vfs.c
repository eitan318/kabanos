#include "arch/i686/errno.h"
#include "fs/fat/fat.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "ksys/stat.h"
#include "mm/kmalloc.h"
#include "vfs.h"
#include "vfs_internal.h"

/* -----------------------------------------------------------------------
 * Per-mount private data (lives in SuperBlock.fs_private)
 * ---------------------------------------------------------------------- */

typedef struct {
  fat_fs_t *fat; /* heap-allocated by fat_mount() */
} fat_sb_priv_t;

/* Convenience: extract fat_fs_t from a SuperBlock */
static inline fat_fs_t *sb_fat(super_block_t *sb) {
  return ((fat_sb_priv_t *)sb->fs_private)->fat;
}

/* -----------------------------------------------------------------------
 * Vnode private data (lives in Vnode.fs_specific)
 * ---------------------------------------------------------------------- */

typedef struct {
  uint32_t first_cluster; /* 0 for FAT12/16 root dir */
  bool is_dir;
  uint32_t size;
} fat_vnode_priv_t;

/* -----------------------------------------------------------------------
 * file_ops – called for open file descriptions
 * ---------------------------------------------------------------------- */

/*
 * fat_vfs_open – called when the VFS opens a file description.
 * We allocate a fat_file_t keyed to the vnode's cluster/size and store it
 * in file->private_data (a per-open-fd slot, not shared with the vnode).
 */
static int fat_vfs_open(file_t *file) {
  vnode_t *vn = file->vnode;
  fat_vnode_priv_t *vp = (fat_vnode_priv_t *)vn->fs_specific;
  fat_fs_t *fat = sb_fat(vn->super_block);

  /* DON'T use path. Use the cluster ID we stored during lookup! */
  fat_file_t *ff =
      fat_open_by_cluster(fat, vp->first_cluster, vp->is_dir, vn->size);
  if (!ff)
    return -ENOENT;

  file->private_data = ff;
  return 0;
}

static ssize_t fat_vfs_read(file_t *file, void *buf, size_t size) {
  fat_file_t *ff = (fat_file_t *)file->private_data;
  if (!ff)
    return -EBADF;
  return fat_read(ff, buf, size);
}

static int fat_vfs_fstat(file_t *file, fstat_t *fstat) {
  fat_file_t *ff = (fat_file_t *)file->private_data;
  if (!ff)
    return -EBADF;
  return fat_fstat(ff, fstat);
}

static off_t fat_vfs_lseek(file_t *file, off_t offset) {
  fat_file_t *ff = (fat_file_t *)file->private_data;
  if (!ff)
    return (off_t)-EBADF;
  return fat_seek(ff, offset);
}

static int fat_vfs_close(file_t *file) {
  fat_file_t *ff = (fat_file_t *)file->private_data;
  if (ff) {
    fat_close(ff);
    file->private_data = NULL;
  }
  return 0;
}

static int fat_vfs_readdir(file_t *file, dir_ctx_t *ctx) {
  fat_file_t *ff = (fat_file_t *)file->private_data;
  if (!ff || !ff->is_directory)
    return -ENOTDIR;

  FAT_DirEntry entry;
  int count = 0;

  while (fat_read_dir(ff, &entry)) {
    /* Convert packed 8.3 name to printable "BASE.EXT" */
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

    uint32_t ino = (uint32_t)entry.first_cluster_low |
                   ((uint32_t)entry.first_cluster_high << 16);
    int type = (entry.attributes & FAT_ATTR_DIRECTORY) ? DT_DIR : DT_REG;

    if (dir_emit(ctx, name, 13, pos, ino, type) < 0)
      break;
    count++;
  }
  return count;
}

/* -----------------------------------------------------------------------
 * vnode_ops – called for name-based operations on vnodes
 * ---------------------------------------------------------------------- */

/*
 * fat_vfs_lookup – resolve one path component within a directory vnode.
 * Returns a new Vnode (ref-counted by VFS) or NULL.
 */
static vnode_t *fat_vfs_lookup(vnode_t *dir_vn, const char *name) {
  fat_vnode_priv_t *dvp = (fat_vnode_priv_t *)dir_vn->fs_specific;
  fat_fs_t *fat = sb_fat(dir_vn->super_block);

  if (!dvp->is_dir)
    return NULL;

  /* 1. Open the current directory vnode as a fat_file_t */
  /* Note: size is 0 for directories in FAT12/16, but that's handled by
   * open_by_cluster */
  fat_file_t *dir_ff =
      fat_open_by_cluster(fat, dvp->first_cluster, true, dvp->size);
  if (!dir_ff)
    return NULL;

  /* 2. Search ONLY this directory for the specific name provided */
  FAT_DirEntry entry;
  if (!dir_find(dir_ff, name, &entry)) {
    fat_close(dir_ff);
    return NULL; // File not found
  }

  /* We are done with the directory handle */
  fat_close(dir_ff);

  /* 3. Prepare the private data for the NEW child vnode */
  fat_vnode_priv_t *vp = kmalloc(sizeof(fat_vnode_priv_t));
  if (!vp)
    return NULL;

  vp->first_cluster = (uint32_t)entry.first_cluster_low |
                      ((uint32_t)entry.first_cluster_high << 16);
  vp->is_dir = (entry.attributes & FAT_ATTR_DIRECTORY) != 0;
  vp->size = entry.size;

  /* 4. Map FAT attributes to VFS mode */
  mode_t mode = vp->is_dir ? (S_IFDIR | 0755) : (S_IFREG | 0644);
  uint32_t ino = vp->first_cluster;

  /* 5. Create the vnode */
  vnode_t *vn = vfs_vnode_alloc(dir_vn->super_block, ino, mode, vp->size, vp);
  if (!vn) {
    kfree(vp);
    return NULL;
  }
  return vn;
}

static void fat_vfs_vnode_free(vnode_t *vn) {
  if (vn->fs_specific) {
    kfree(vn->fs_specific);
    vn->fs_specific = NULL;
  }
}

/* -----------------------------------------------------------------------
 * Filesystem type registration
 * ---------------------------------------------------------------------- */

static int fat_kill_super(super_block_t *sb) {
  fat_sb_priv_t *priv = (fat_sb_priv_t *)sb->fs_private;
  if (priv) {
    fat_unmount(priv->fat);
    kfree(priv);
    sb->fs_private = NULL;
  }
  return 0;
}

/* Op tables – file-scoped so the registration below can take their address */
static struct file_ops fat_file_ops = {
    .open = fat_vfs_open,
    .read = fat_vfs_read,
    .seek = fat_vfs_lseek,
    .fstat = fat_vfs_fstat,
    .close = fat_vfs_close,
    .iter_dir = fat_vfs_readdir,
    /* write / ioctl / mmap = NULL (read-only for now) */
};

static struct vnode_ops fat_vnode_ops = {
    .lookup = fat_vfs_lookup, .destroy = fat_vfs_vnode_free,
    /* create / mkdir / unlink / rename = NULL (read-only) */
};

static int fat_fill_super(super_block_t *sb, blkdev_t *dev) {
  fat_fs_t *fat = fat_mount(dev);
  if (!fat)
    return -EIO;

  fat_sb_priv_t *priv = kmalloc(sizeof(fat_sb_priv_t));
  if (!priv) {
    fat_unmount(fat);
    return -ENOMEM;
  }
  priv->fat = fat;
  sb->fs_private = priv;

  sb->f_ops = &fat_file_ops;
  sb->v_ops = &fat_vnode_ops;

  /* Allocate root vnode */
  fat_vnode_priv_t *root_vp = kmalloc(sizeof(fat_vnode_priv_t));
  if (!root_vp) {
    kfree(priv);
    fat_unmount(fat);
    return -ENOMEM;
  }
  root_vp->first_cluster = (fat->type == FAT_TYPE_32) ? fat->root_cluster : 0;
  root_vp->is_dir = true;
  root_vp->size = 0;

  vnode_t *root =
      vfs_vnode_alloc(sb, root_vp->first_cluster, S_IFDIR | 0755, 0, root_vp);
  if (!root) {
    kfree(root_vp);
    kfree(priv);
    fat_unmount(fat);
    return -ENOMEM;
  }

  sb->fs_root = root;

  kdebugf("fat_vfs: mounted FAT%d filesystem\n", (int)fat->type);
  return 0;
}

static fs_type_t fat_fs_type = {
    .name = "fat",
    .fill_sb = fat_fill_super,
    .kill_sb = fat_kill_super,
    .next = NULL,
};

int fat_init(module_t *module) {
  vfs_fs_type_register(&fat_fs_type);
  return 0;
}

static const char *fat_deps[] = {"ata", NULL};

ITER_MODULE(fat) = {
    .name = "keyboard",
    .required = fat_deps,
    .init = &fat_init,
    .fini = NULL,
};

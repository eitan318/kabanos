#include "fs/fs_common.h"
#include "fs/myfs/myfs.h"
#include <dirent.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SECTOR_BYTES 512

/* -----------------------------------------------------------------------
 * FIX 1: HostDevice now carries block_bytes so read/write use the correct
 *         transfer size instead of the hard-coded SECTOR_BYTES (512).
 * ---------------------------------------------------------------------- */
typedef struct {
  FILE *fp;
  size_t offset_bytes;
  size_t size_bytes;
  size_t block_bytes; /* FIX 1: full myfs block size, e.g. 4 * 512 = 2048 */
} HostDevice;

static int host_read_block(void *dev_handle, uint32_t lba, void *buf) {
  HostDevice *dev = (HostDevice *)dev_handle;
  /* FIX 1: stride by block_bytes, not SECTOR_BYTES */
  size_t absolute_pos = dev->offset_bytes + ((size_t)lba * dev->block_bytes);
  if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  /* FIX 1: read a full block, not just one sector */
  return (fread(buf, dev->block_bytes, 1, dev->fp) == 1) ? 0 : -1;
}

static int host_write_block(void *dev_handle, uint32_t lba, const void *buf) {
  HostDevice *dev = (HostDevice *)dev_handle;
  /* FIX 1: stride by block_bytes, not SECTOR_BYTES */
  size_t absolute_pos = dev->offset_bytes + ((size_t)lba * dev->block_bytes);
  if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  /* FIX 1: write a full block, not just one sector */
  if (fwrite(buf, dev->block_bytes, 1, dev->fp) != 1)
    return -1;
  fflush(dev->fp);
  return 0;
}

/* -----------------------------------------------------------------------
 * FIX 5: proper variadic wrapper instead of casting printf to void*.
 * ---------------------------------------------------------------------- */
static void myfs_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

fs_platform_t *host_platform_create(void) {
  fs_platform_t *plt = malloc(sizeof(fs_platform_t));
  if (!plt)
    return NULL;
  plt->alloc = malloc;
  plt->free = free;
  plt->read_block = host_read_block;
  plt->write_block = host_write_block;
  plt->log = myfs_log; /* FIX 5: no more (void*)printf cast */
  return plt;
}

/* -----------------------------------------------------------------------
 * Copy a regular file from the host into the myfs
 * ---------------------------------------------------------------------- */
static int copy_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                     const char *name, const char *host_path) {
  FILE *f = fopen(host_path, "rb");
  if (!f) {
    fprintf(stderr, "  [!] cannot open host file: %s\n", host_path);
    return -1;
  }

  uint32_t new_ino;
  if (myfs_create_file(sb, parent_dir, name, &new_ino) < 0) {
    fprintf(stderr, "  [!] myfs_create_file failed for: %s\n", name);
    fclose(f);
    return -1;
  }

  MyfsInode *inode = myfs_iget(sb, new_ino);
  if (!inode) {
    fclose(f);
    return -1;
  }

  uint8_t buf[4096];
  uint32_t offset = 0;
  size_t bytes_read;
  while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
    if (myfs_node_write(sb, inode, offset, buf, bytes_read) < 0) {
      fprintf(stderr, "  [!] write failed at offset %u in: %s\n", offset, name);
      break;
    }
    offset += bytes_read;
  }

  printf("  [F] %s (%u bytes)\n", name, offset);
  myfs_iput(sb, inode);
  fclose(f);
  return 0;
}

/* -----------------------------------------------------------------------
 * Recursively copy host_path directory into fs_parent_dir
 * ---------------------------------------------------------------------- */
static int populate_from_dir(MyfsSuperBlock *sb, MyfsInode *fs_parent_dir,
                             const char *host_path, int depth) {
  DIR *d = opendir(host_path);
  if (!d) {
    fprintf(stderr, "[!] cannot opendir: %s\n", host_path);
    return -1;
  }

  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    /* skip . and .. */
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    /* build full host path */
    char child_host_path[4096];
    snprintf(child_host_path, sizeof(child_host_path), "%s/%s", host_path,
             ent->d_name);

    struct stat st;
    /* FIX 3: lstat() instead of stat() so S_ISLNK is reachable */
    if (lstat(child_host_path, &st) < 0) {
      fprintf(stderr, "[!] lstat failed: %s\n", child_host_path);
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      for (int i = 0; i < depth; i++)
        printf("  ");
      printf("[D] %s/\n", ent->d_name);

      uint32_t new_dir_ino;
      if (myfs_create_dir(sb, fs_parent_dir, ent->d_name, &new_dir_ino) < 0) {
        fprintf(stderr, "[!] myfs_create_dir failed: %s\n", ent->d_name);
        continue;
      }

      MyfsInode *child_dir = myfs_iget(sb, new_dir_ino);
      if (!child_dir)
        continue;
      populate_from_dir(sb, child_dir, child_host_path, depth + 1);
      myfs_iput(sb, child_dir);

    } else if (S_ISLNK(st.st_mode)) {
      /* FIX 3: this branch is now reachable because we use lstat() */
      char link_target[4096];
      ssize_t len =
          readlink(child_host_path, link_target, sizeof(link_target) - 1);
      if (len < 0)
        continue;
      link_target[len] = '\0';

      for (int i = 0; i < depth; i++)
        printf("  ");
      printf("[L] %s -> %s\n", ent->d_name, link_target);
      myfs_create_symlink(sb, fs_parent_dir, ent->d_name, link_target);

    } else if (S_ISREG(st.st_mode)) {
      for (int i = 0; i < depth; i++)
        printf("  ");
      copy_file(sb, fs_parent_dir, ent->d_name, child_host_path);
    }
    /* other types (sockets, devices) are silently skipped */
  }

  closedir(d);
  return 0;
}

/* -----------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
  if (argc < 4 || argc > 5) {
    fprintf(stderr,
            "usage: %s <img> <offset_bytes> <size_bytes> [partroot_dir]\n",
            argv[0]);
    return 1;
  }

  const char *partroot = (argc == 5) ? argv[4] : NULL;

  HostDevice dev;
  dev.fp = fopen(argv[1], "r+b");
  if (!dev.fp) {
    perror("Failed to open image");
    return 1;
  }
  dev.offset_bytes = (size_t)strtoull(argv[2], NULL, 0);
  dev.size_bytes = (size_t)strtoull(argv[3], NULL, 0);
  /* FIX 1: initialise block_bytes before the platform is used.
   *        This must match MYFS_BLOCK_SECTORS * SECTOR_BYTES in myfs_format.
   *        (4 sectors * 512 bytes = 2048 bytes per block)               */
  dev.block_bytes = 4 * SECTOR_BYTES;

  fs_platform_t *host_plt = host_platform_create();
  if (!host_plt) {
    fprintf(stderr, "host_platform_create failed\n");
    fclose(dev.fp);
    return 1;
  }

  if (myfs_format(&dev, host_plt) < 0) {
    fprintf(stderr, "myfs_format failed\n");
    fclose(dev.fp);
    free(host_plt);
    return 1;
  }

  /* FIX 4: divide by block_bytes (not SECTOR_BYTES) for an accurate count */
  uint32_t total_blocks = (uint32_t)(dev.size_bytes / dev.block_bytes);
  printf("Formatted: %s (%u blocks of %zu bytes)\n", argv[1], total_blocks,
         dev.block_bytes);

  if (partroot) {
    printf("Populating from: %s\n", partroot);

    MyfsSuperBlock *sb = myfs_sb_read(&dev, host_plt);
    if (!sb) {
      fprintf(stderr, "myfs_sb_read failed\n");
      fclose(dev.fp);
      free(host_plt);
      return 1;
    }

    /* FIX 2: myfs_format leaves the root inode unallocated.
     *        Allocate it explicitly here before trying to iget it.      */
    MyfsInode *root = NULL;
    uint32_t root_ino;
    if (myfs_inode_alloc(sb, &root, S_IFDIR | 0755) < 0) {
      fprintf(stderr, "cannot allocate root inode\n");
      myfs_sb_kill(sb);
      fclose(dev.fp);
      free(host_plt);
      return 1;
    }
    root_ino = root->i_ino;

    /* Optionally add the canonical . and .. self-references */
    myfs_dir_add_entry(sb, root, ".", root_ino);
    myfs_dir_add_entry(sb, root, "..", root_ino);

    populate_from_dir(sb, root, partroot, 0);

    myfs_iput(sb, root);
    myfs_sb_kill(sb); /* flushes everything to disk */
    printf("Done. Root inode = %u\n", root_ino);
  }

  fclose(dev.fp);
  free(host_plt);
  return 0;
}

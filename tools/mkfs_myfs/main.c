#include "myfs.h"
#include <dirent.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SECTOR_BYTES 512

typedef struct {
  uint32_t sectors;
  FILE *fp;
} mydev_t;

static int host_read_sectors(blkdev_t *dev, uint32_t lba, uint32_t count,
                             void *buf) {
  size_t absolute_pos = lba * SECTOR_BYTES;
  mydev_t *mydev = (mydev_t *)dev->priv;
  if (fseek(mydev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  return (fread(buf, SECTOR_BYTES * count, 1, mydev->fp) == 1) ? 0 : -1;
}

static int host_write_sectors(blkdev_t *dev, uint32_t lba, uint32_t count,
                              const void *buf) {
  size_t absolute_pos = lba * SECTOR_BYTES;
  mydev_t *mydev = (mydev_t *)dev->priv;
  if (fseek(mydev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  if (fwrite(buf, SECTOR_BYTES * count, 1, mydev->fp) != 1)
    return -1;
  fflush(mydev->fp);
  return 0;
}

static void myfs_log(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

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

static int populate_from_dir(MyfsSuperBlock *sb, MyfsInode *fs_parent_dir,
                             const char *host_path, int depth) {
  DIR *d = opendir(host_path);
  if (!d) {
    fprintf(stderr, "[!] cannot opendir: %s\n", host_path);
    return -1;
  }

  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;

    char child_host_path[4096];
    snprintf(child_host_path, sizeof(child_host_path), "%s/%s", host_path,
             ent->d_name);

    struct stat st;
    if (lstat(child_host_path, &st) < 0) {
      fprintf(stderr, "[!] lstat failed: %s\n", child_host_path);
      return -1;
    }

    if (S_ISDIR(st.st_mode)) {
      for (int i = 0; i < depth; i++)
        printf("  ");
      printf("[D] %s/\n", ent->d_name);

      uint32_t new_dir_ino;
      if (myfs_create_dir(sb, fs_parent_dir, ent->d_name, &new_dir_ino) < 0) {
        fprintf(stderr, "[!] myfs_create_dir failed: %s\n", ent->d_name);
        return -1;
      }

      MyfsInode *child_dir = myfs_iget(sb, new_dir_ino);
      if (!child_dir)
        return -1;

      populate_from_dir(sb, child_dir, child_host_path, depth + 1);
      myfs_iput(sb, child_dir);

    } else if (S_ISREG(st.st_mode)) {
      for (int i = 0; i < depth; i++)
        printf("  ");
      copy_file(sb, fs_parent_dir, ent->d_name, child_host_path);
    }
  }

  closedir(d);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <img> <size_sectors> [partroot_dir]\n", argv[0]);
    return 1;
  }

  blkdev_t dev;

  mydev_t *mydev = malloc(sizeof(*mydev));
  if (!dev.priv) {
    perror("Failed to open image");
    return 1;
  }
  mydev->fp = fopen(argv[1], "r+b");
  mydev->sectors = (size_t)strtoull(argv[2], NULL, 0);

  memset(&dev, 0, sizeof(dev)); // Clear garbage
  dev.priv = mydev;
  dev.read_sectors = host_read_sectors;   // Point to your host functions
  dev.write_sectors = host_write_sectors; // Point to your host functions

  if (myfs_format(&dev, mydev->sectors) < 0) {
    fprintf(stderr, "myfs_format failed\n");
    fclose(mydev->fp);
    return 1;
  }
  printf("Formatted: %s (%u sectors)\n", argv[1], mydev->sectors);

  const char *partroot = argv[3];

  MyfsSuperBlock *sb = myfs_sb_read(&dev);
  MyfsInode *root = myfs_iget(sb, sb->on_disk.root_inode);
  if (root == NULL) {
    return -1;
  }

  printf("[HEREU2]");
  populate_from_dir(sb, root, partroot, 0);

  myfs_iput(sb, root);
  myfs_sb_kill(sb);

  return 0;
}

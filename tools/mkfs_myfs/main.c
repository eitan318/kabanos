// #include "fs/fs_common.h"
// #include "fs/myfs/myfs.h"
// #include <dirent.h>
// #include <stdarg.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
//
// #define SECTOR_BYTES 512
//
// typedef struct {
//   FILE *fp;
//   size_t offset_bytes;
//   size_t size_bytes;
//   size_t block_bytes;
// } HostDevice;
//
// static int host_read_block(void *dev_handle, uint32_t lba, void *buf) {
//   HostDevice *dev = (HostDevice *)dev_handle;
//   size_t absolute_pos = dev->offset_bytes + ((size_t)lba * dev->block_bytes);
//   if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
//     return -1;
//   return (fread(buf, dev->block_bytes, 1, dev->fp) == 1) ? 0 : -1;
// }
//
// static int host_write_block(void *dev_handle, uint32_t lba, const void *buf)
// {
//   HostDevice *dev = (HostDevice *)dev_handle;
//   size_t absolute_pos = dev->offset_bytes + ((size_t)lba * dev->block_bytes);
//   if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
//     return -1;
//   if (fwrite(buf, dev->block_bytes, 1, dev->fp) != 1)
//     return -1;
//   fflush(dev->fp);
//   return 0;
// }
//
// static void myfs_log(const char *fmt, ...) {
//   va_list ap;
//   va_start(ap, fmt);
//   vprintf(fmt, ap);
//   va_end(ap);
// }
//
// fs_platform_t *host_platform_create(void) {
//   fs_platform_t *plt = malloc(sizeof(fs_platform_t));
//   if (!plt)
//     return NULL;
//   plt->alloc = malloc;
//   plt->free = free;
//   plt->read_block = host_read_block;
//   plt->write_block = host_write_block;
//   plt->log = myfs_log;
//   return plt;
// }
//
// static int copy_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
//                      const char *name, const char *host_path) {
//   FILE *f = fopen(host_path, "rb");
//   if (!f) {
//     fprintf(stderr, "  [!] cannot open host file: %s\n", host_path);
//     return -1;
//   }
//
//   uint32_t new_ino;
//   if (myfs_create_file(sb, parent_dir, name, &new_ino) < 0) {
//     fprintf(stderr, "  [!] myfs_create_file failed for: %s\n", name);
//     fclose(f);
//     return -1;
//   }
//
//   MyfsInode *inode = myfs_iget(sb, new_ino);
//   if (!inode) {
//     fclose(f);
//     return -1;
//   }
//
//   uint8_t buf[4096];
//   uint32_t offset = 0;
//   size_t bytes_read;
//   while ((bytes_read = fread(buf, 1, sizeof(buf), f)) > 0) {
//     if (myfs_node_write(sb, inode, offset, buf, bytes_read) < 0) {
//       fprintf(stderr, "  [!] write failed at offset %u in: %s\n", offset,
//       name); break;
//     }
//     offset += bytes_read;
//   }
//
//   printf("  [F] %s (%u bytes)\n", name, offset);
//   myfs_iput(sb, inode);
//   fclose(f);
//   return 0;
// }
//
// static int populate_from_dir(MyfsSuperBlock *sb, MyfsInode *fs_parent_dir,
//                              const char *host_path, int depth) {
//   DIR *d = opendir(host_path);
//   if (!d) {
//     fprintf(stderr, "[!] cannot opendir: %s\n", host_path);
//     return -1;
//   }
//
//   struct dirent *ent;
//   while ((ent = readdir(d)) != NULL) {
//     if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
//       continue;
//
//     char child_host_path[4096];
//     snprintf(child_host_path, sizeof(child_host_path), "%s/%s", host_path,
//              ent->d_name);
//
//     struct stat st;
//     if (lstat(child_host_path, &st) < 0) {
//       fprintf(stderr, "[!] lstat failed: %s\n", child_host_path);
//       return -1;
//     }
//
//     if (S_ISDIR(st.st_mode)) {
//       for (int i = 0; i < depth; i++)
//         printf("  ");
//       printf("[D] %s/\n", ent->d_name);
//
//       uint32_t new_dir_ino;
//       if (myfs_create_dir(sb, fs_parent_dir, ent->d_name, &new_dir_ino) < 0)
//       {
//         fprintf(stderr, "[!] myfs_create_dir failed: %s\n", ent->d_name);
//         return -1;
//       }
//
//       MyfsInode *child_dir = myfs_iget(sb, new_dir_ino);
//       if (!child_dir)
//         return -1;
//
//       populate_from_dir(sb, child_dir, child_host_path, depth + 1);
//       myfs_iput(sb, child_dir);
//
//     } else if (S_ISLNK(st.st_mode)) {
//       char link_target[4096];
//       ssize_t len =
//           readlink(child_host_path, link_target, sizeof(link_target) - 1);
//       if (len < 0)
//         return -1;
//       link_target[len] = '\0';
//
//       for (int i = 0; i < depth; i++)
//         printf("  ");
//       printf("[L] %s -> %s\n", ent->d_name, link_target);
//       myfs_create_symlink(sb, fs_parent_dir, ent->d_name, link_target);
//
//     } else if (S_ISREG(st.st_mode)) {
//       for (int i = 0; i < depth; i++)
//         printf("  ");
//       copy_file(sb, fs_parent_dir, ent->d_name, child_host_path);
//     }
//   }
//
//   closedir(d);
//   return 0;
// }
//
// int main(int argc, char *argv[]) {
//   if (argc < 4 || argc > 5) {
//     fprintf(stderr,
//             "usage: %s <img> <offset_bytes> <size_bytes> [partroot_dir]\n",
//             argv[0]);
//     return 1;
//   }
//
//   const char *partroot = (argc == 5) ? argv[4] : NULL;
//
//   HostDevice dev;
//   dev.fp = fopen(argv[1], "r+b");
//   if (!dev.fp) {
//     perror("Failed to open image");
//     return 1;
//   }
//   dev.offset_bytes = (size_t)strtoull(argv[2], NULL, 0);
//   dev.size_bytes = (size_t)strtoull(argv[3], NULL, 0);
//   dev.block_bytes = 4 * SECTOR_BYTES;
//
//   printf("Tool writing block 1 at file offset: %zu\n",
//          dev.offset_bytes + 1 * dev.block_bytes);
//   printf("Tool writing block 17 at file offset: %zu\n",
//          dev.offset_bytes + 17 * dev.block_bytes);
//
//   fs_platform_t *host_plt = host_platform_create();
//   if (!host_plt) {
//     fprintf(stderr, "host_platform_create failed\n");
//     fclose(dev.fp);
//     return 1;
//   }
//
//   uint32_t total_blocks = (uint32_t)(dev.size_bytes / dev.block_bytes);
//
//   if (myfs_format(&dev, total_blocks) < 0) {
//     fprintf(stderr, "myfs_format failed\n");
//     fclose(dev.fp);
//     free(host_plt);
//     return 1;
//   }
//   printf("Formatted: %s (%u blocks of %zu bytes)\n", argv[1], total_blocks,
//          dev.block_bytes);
//
//   if (partroot) {
//     MyfsSuperBlock *sb = myfs_sb_read(&dev, host_plt);
//     MyfsInode *root = myfs_iget(sb, sb->on_disk.root_inode);
//     if (root == NULL) {
//       return -1;
//     }
//     populate_from_dir(sb, root, partroot, 0);
//
//     myfs_iput(sb, root);
//     myfs_sb_kill(sb);
//   }
//
//   return 0;
// }

#pragma once

#include "drivers/block/blockdev.h"
#include "fs/myfs/myfs_format.h"
#include "klib/stddef.h"

#define SECTOR_BYTES 512

#define MYFS_NAME "myfs"
#define MYFS_ROOT_INODE_NUM 0
#define MYFS_DIRECT_BLOCKS_MAX 12

typedef struct {
  uint32_t i_ino;
  uint32_t direct_blocks[MYFS_DIRECT_BLOCKS_MAX];
  int block_count;
  uint32_t size;
  int mode;
  uint32_t ref_count;
  uint32_t links_count;
  uint32_t uid;
  uint32_t gid;
  uint32_t permissions;
  uint64_t atime;
  uint64_t mtime;
  uint64_t ctime;
  int dirty;
} MyfsInode;

#define INODE_HASH_SIZE 256
#define INODE_HASH(ino) ((ino) % INODE_HASH_SIZE)

typedef struct inode_hash_entry {
  MyfsInode *inode;
  struct inode_hash_entry *next;
} InodeHashEntry;

typedef struct {
  MyfsDiskSuperBlock on_disk;
  blkdev_t *dev;
  uint32_t block_bytes;
  uint8_t *block_bitmap;
  uint8_t *inode_bitmap;
  InodeHashEntry *inode_hash_table[INODE_HASH_SIZE];
  int mounted;
} MyfsSuperBlock;

int myfs_format(blkdev_t *dev);
MyfsSuperBlock *myfs_sb_read(blkdev_t *dev);

int myfs_sb_kill(MyfsSuperBlock *sb);

int myfs_rename(MyfsSuperBlock *sb, MyfsInode *old_parent_inode,
                const char *old_name, MyfsInode *new_parent_inode,
                const char *new_name);

ssize_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsInode *inode, char *buf,
                          size_t bufsize);

int myfs_create_symlink(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                        const char *name, const char *target);

int myfs_readdir(MyfsSuperBlock *sb, MyfsInode *dir_inode, uint32_t offset,
                 MyfsDiskDirEntry *entry);

MyfsInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino);

int myfs_disk_inode_write(MyfsSuperBlock *sb, int ino);

MyfsInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino);

void myfs_iput(MyfsSuperBlock *sb, MyfsInode *inode);

int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsInode **inode, int mode);

ssize_t myfs_node_read(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                       void *buf, size_t count);

ssize_t myfs_node_write(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                        const void *buf, size_t count);

int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsInode *inode,
                        uint32_t new_size);

int myfs_lookup(MyfsSuperBlock *sb, MyfsInode *dir_inode, const char *name,
                uint32_t *found_ino);

int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                       const char *name, uint32_t inode_num);

int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                      const char *name);

int myfs_create_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                     const char *name, uint32_t *new_ino);

int myfs_create_dir(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name,
                    uint32_t *new_ino);

int myfs_unlink(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name);

int myfs_entry_idx_find(MyfsDiskDirEntry *entries, int entries_count,
                        const char *name);

const char *myfs_version_get(void);

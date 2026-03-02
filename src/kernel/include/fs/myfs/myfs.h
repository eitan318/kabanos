#pragma once

#include "fs/fs_common.h"
#include "fs/myfs/myfs_layout.h"
#include "klib/stdbool.h"
#include "klib/string.h"
#include "utils/math.h"

#define SECTOR_BYTES 512

#define MYFS_NAME "myfs"
#define MYFS_ROOT_INODE_NUM 1
#define MYFS_DIRECT_BLOCKS_MAX 12

#define INODE_HASH_SIZE 256
#define INODE_HASH(ino) ((ino) % INODE_HASH_SIZE)

typedef struct inode_hash_entry {
  MyfsDiskInode *inode;
  struct inode_hash_entry *next;
} InodeHashEntry;

typedef struct {
  MyfsDiskSuperBlock on_disk;
  void *dev;
  uint32_t block_bytes;
  uint8_t *block_bitmap;
  uint8_t *inode_bitmap;
  InodeHashEntry *inode_hash_table[INODE_HASH_SIZE];
  int mounted;
  fs_platform_t *plt;
} MyfsSuperBlock;

int myfs_format(void *dev, fs_platform_t *plt, int part_blocks);

MyfsSuperBlock *myfs_sb_read(void *dev, fs_platform_t *plt);

int myfs_sb_kill(MyfsSuperBlock *sb);

int myfs_rename(MyfsSuperBlock *sb, MyfsDiskInode *old_parent_inode,
                const char *old_name, MyfsDiskInode *new_parent_inode,
                const char *new_name);

uint32_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsDiskInode *inode, char *buf,
                           size_t bufsize);

int myfs_create_symlink(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode,
                        const char *name, const char *target);

int myfs_readdir(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode, uint32_t offset,
                 MyfsDiskDirEntry *entry);

MyfsDiskInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino);

int myfs_disk_inode_write(MyfsSuperBlock *sb, int ino);

MyfsDiskInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino);

void myfs_iput(MyfsSuperBlock *sb, MyfsDiskInode *inode);

int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsDiskInode **inode, int mode);

uint32_t myfs_node_read(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                        uint32_t offset, void *buf, size_t count);

uint32_t myfs_node_write(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                         uint32_t offset, const void *buf, size_t count);

int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                        uint32_t new_size);

int myfs_lookup(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode, const char *name,
                uint32_t *found_ino);

int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode,
                       const char *name, uint32_t inode_num);

int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode,
                      const char *name);

int myfs_create_file(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                     const char *name, uint32_t *new_ino);

int myfs_create_dir(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                    const char *name, uint32_t *new_ino);

int myfs_unlink(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                const char *name);

int myfs_entry_idx_find(MyfsDiskDirEntry *entries, int entries_count,
                        const char *name);

const char *myfs_version_get(void);

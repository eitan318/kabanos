#pragma once

#include <klib/stdint.h>

#define MYFS_MAGIC 0x1001BEEF
#define MYFS_DIRECT_BLOCKS_MAX 12

typedef struct {
  char file_name[32];
  uint32_t inode_num;
} __attribute__((packed)) MyfsDiskDirEntry;

typedef struct {
  uint32_t i_ino;
  uint32_t direct_blocks[MYFS_DIRECT_BLOCKS_MAX];
  uint32_t block_count;
  uint32_t size;
  uint32_t mode;
  uint32_t ref_count;
  uint32_t links_count;
  uint32_t uid;
  uint32_t gid;
  uint32_t permissions;
  uint64_t atime;
  uint64_t mtime;
  uint64_t ctime;
} __attribute__((packed)) MyfsDiskInode;

typedef struct {
  uint32_t magic;
  uint32_t total_inodes;
  uint32_t total_blocks;
  uint32_t free_inodes;
  uint32_t free_blocks;
  uint32_t block_sectors;
  uint32_t file_initial_blocks;
  uint32_t inode_bitmap_start;
  uint32_t block_bitmap_start;
  uint32_t inode_table_start;
  uint32_t data_blocks_start;
  uint32_t root_inode;
} __attribute__((packed)) MyfsDiskSuperBlock;

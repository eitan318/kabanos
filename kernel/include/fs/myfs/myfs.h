/**
 * @file myfs.h
 * @brief MyFS: the native filesystem (superblock, inodes, directories).
 *
 * This layer has no VFS dependencies; it is also linked into the host
 * mkfs tool (built with MKFS_MYFS defined).
 */
#pragma once

#ifdef MKFS_MYFS
#include "../../drivers/block/blockdev.h"
#else
#include "drivers/block/blockdev.h"
#include "klib/stdint.h"
#endif

#define SECTOR_BYTES 512

#define MYFS_MAGIC 0x20250928
#define MYFS_NAME "myfs"
#define MYFS_ROOT_INODE_NUM 0
#define MYFS_DIRECT_BLOCKS_MAX 10

/** @brief On-disk directory entry. */
typedef struct {
  char file_name[32]; /**< Entry name (max 31 chars + null terminator). */
  uint32_t inode_num; /**< Inode number of the entry. */
} __attribute__((packed)) MyfsDirEntry;

/** @brief Inode structure (on-disk layout, also used in memory). */
typedef struct {
  uint32_t i_ino;                              /**< Inode number. */
  uint32_t block_ptrs[MYFS_DIRECT_BLOCKS_MAX]; /**< Direct block pointers. */
  uint32_t single_indirect_ptr; /**< Block holding further block pointers. */
  uint32_t double_indirect_ptr; /**< Block holding single-indirect blocks. */
  uint32_t block_count;         /**< Number of blocks allocated. */
  uint32_t size;                /**< File/directory size in bytes. */
  uint32_t mode;                /**< File type (S_IFREG, S_IFDIR, ...). */
  uint32_t ref_count;           /**< In-memory reference count. */
  uint32_t links_count;         /**< Hard link count. */
  uint32_t uid;                 /**< Owner user ID. */
  uint32_t gid;                 /**< Owner group ID. */
  uint32_t permissions;         /**< Permission bits (e.g. 0755). */
  uint64_t atime;               /**< Access time (Unix timestamp). */
  uint64_t mtime;               /**< Modification time (Unix timestamp). */
  uint64_t ctime;               /**< Creation time (Unix timestamp). */
  uint32_t dirty;               /**< Nonzero if it must be written back. */
} __attribute__((packed)) MyfsInode;

/** @brief On-disk superblock. */
typedef struct {
  uint32_t magic;               /**< MYFS_MAGIC. */
  uint32_t total_inodes;        /**< Total number of inodes. */
  uint32_t total_blocks;        /**< Total number of blocks. */
  uint32_t free_inodes;         /**< Currently unused. */
  uint32_t free_blocks;         /**< Currently unused. */
  uint32_t block_sectors;       /**< Sectors per block. */
  uint32_t file_initial_blocks; /**< Blocks pre-allocated for new files. */
  uint32_t inode_bitmap_start;  /**< First block of the inode bitmap. */
  uint32_t block_bitmap_start;  /**< First block of the block bitmap. */
  uint32_t inode_table_start;   /**< First block of the inode table. */
  uint32_t data_blocks_start;   /**< First block of the data area. */
  uint32_t root_inode;          /**< Root directory inode number. */
} MyfsDiskSuperBlock;

#define INODE_HASH_SIZE 256
#define INODE_HASH(ino) ((ino) % INODE_HASH_SIZE)

/** @brief Inode cache hash bucket entry (chained on collision). */
typedef struct inode_hash_entry {
  MyfsInode *inode;
  struct inode_hash_entry *next;
} InodeHashEntry;

/** @brief In-memory superblock. */
typedef struct {
  MyfsDiskSuperBlock on_disk; /**< Copy of the on-disk superblock. */
  blkdev_t *dev;              /**< Backing block device. */
  uint32_t block_bytes;       /**< Block size in bytes. */
  uint8_t *block_bitmap;      /**< In-memory block allocation bitmap. */
  uint8_t *inode_bitmap;      /**< In-memory inode allocation bitmap. */
  InodeHashEntry *inode_hash_table[INODE_HASH_SIZE]; /**< Inode cache. */
  uint32_t mounted;                                  /**< Mounted flag. */
} MyfsSuperBlock;

/**
 * @brief Formats a disk with the MyFS filesystem.
 *
 * Creates the superblock, bitmaps and inode table structures.
 *
 * @return 0 on success, negative on error.
 */
int myfs_format(blkdev_t *dev, uint32_t max_blocks);

/**
 * @brief Reads the superblock from disk and initializes the in-memory
 *        structures.
 * @return Pointer to superblock on success, NULL on error.
 */
MyfsSuperBlock *myfs_sb_read(blkdev_t *dev);

/**
 * @brief Unmounts the filesystem: flushes all dirty data and frees @p sb.
 * @return 0 on success.
 */
int myfs_sb_kill(MyfsSuperBlock *sb);

/**
 * @brief Renames/moves a file or directory.
 * @return 0 on success, negative on error.
 */
int myfs_rename(MyfsSuperBlock *sb, MyfsInode *old_parent_inode,
                const char *old_name, MyfsInode *new_parent_inode,
                const char *new_name);

/**
 * @brief Reads the target path of a symbolic link into @p buf.
 * @return Number of bytes read, or negative on error.
 */
ssize_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsInode *inode, char *buf,
                          size_t bufsize);

/**
 * @brief Creates a symbolic link named @p name pointing to @p target.
 * @return 0 on success, negative on error.
 */
int myfs_create_symlink(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                        const char *name, const char *target);

/**
 * @brief Reads the directory entry at byte offset @p offset.
 * @return Number of bytes read, 0 at end of directory, negative on error.
 */
int myfs_readdir(MyfsSuperBlock *sb, MyfsInode *dir_inode, uint32_t offset,
                 MyfsDirEntry *entry);

/**
 * @brief Reads inode @p ino from disk, bypassing the cache.
 * @return Newly allocated inode, or NULL on error.
 */
MyfsInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino);

/**
 * @brief Writes inode @p ino back to disk.
 * @return 0 on success, negative on error.
 */
int myfs_disk_inode_write(MyfsSuperBlock *sb, int ino);

/**
 * @brief Gets an inode from the cache, reading it from disk on a miss.
 *
 * Increments the reference count; must be balanced with myfs_iput().
 *
 * @return Pointer to inode, or NULL on error.
 */
MyfsInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino);

/**
 * @brief Releases an inode reference; writes it back if dirty when the
 *        reference count reaches zero.
 */
void myfs_iput(MyfsSuperBlock *sb, MyfsInode *inode);

/**
 * @brief Allocates a new inode with the given mode.
 * @param inode [out] Receives the newly allocated inode.
 * @return 0 on success, negative on error.
 */
int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsInode **inode, int mode);

/**
 * @brief Reads @p count bytes of file data starting at @p offset.
 * @return Number of bytes read, or negative on error.
 */
ssize_t myfs_node_read(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                       void *buf, size_t count);

/**
 * @brief Writes @p count bytes of file data starting at @p offset,
 *        allocating blocks as needed.
 * @return Number of bytes written, or negative on error.
 */
ssize_t myfs_node_write(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                        const void *buf, size_t count);

/**
 * @brief Changes an inode's size, freeing blocks when shrinking.
 * @return 0 on success, negative on error.
 */
int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsInode *inode,
                        uint32_t new_size);

/**
 * @brief Looks up @p name in a directory.
 * @param found_ino [out] Receives the entry's inode number.
 * @return 0 on success, negative if not found or on error.
 */
int myfs_lookup(MyfsSuperBlock *sb, MyfsInode *dir_inode, const char *name,
                uint32_t *found_ino);

/**
 * @brief Adds an entry to a directory.
 * @return 0 on success, negative on error.
 */
int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                       const char *name, uint32_t inode_num);

/**
 * @brief Removes an entry from a directory.
 * @return 0 on success, negative on error.
 */
int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                      const char *name);

/**
 * @brief Creates a new regular file.
 * @param new_ino [out] Receives the new inode number.
 * @return 0 on success, negative on error.
 */
int myfs_create_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                     const char *name, uint32_t *new_ino);

/**
 * @brief Creates a new directory with "." and ".." entries.
 * @param new_ino [out] Receives the new inode number.
 * @return 0 on success, negative on error.
 */
int myfs_create_dir(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name,
                    uint32_t *new_ino);

/**
 * @brief Deletes a file from a directory.
 * @return 0 on success, negative on error.
 */
int myfs_unlink(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name);

/**
 * @brief Deletes an (empty) directory.
 * @return 0 on success, negative on error.
 */
int myfs_remove_dir(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                    const char *name);

/**
 * @brief Finds the index of a named entry in an entry array.
 * @return Index of entry, or -1 if not found.
 */
int myfs_entry_idx_find(MyfsDirEntry *entries, int entries_count,
                        const char *name);

/** @brief Returns the filesystem version string. */
const char *myfs_version_get(void);

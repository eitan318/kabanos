#pragma once

#include "klib/stddef.h"
#include "klib/stdint.h"
#include "ksys/stat.h"

#define SECTOR_BYTES 512

#define MYFS_MAGIC 0x20250928
#define MYFS_NAME "myfs"
#define MYFS_ROOT_INODE_NUM 0
#define MYFS_DIRECT_BLOCKS_MAX 12

/**
 * struct MyfsDirEntry - Directory entry structure
 * @file_name: Name of the file/directory (max 31 chars + null terminator)
 * @inode_num: Inode number of the entry
 */
typedef struct {
  char file_name[32];
  uint32_t inode_num;
} MyfsDirEntry;

/**
 * struct MyfsInode - Filesystem inode structure
 * @i_ino: Inode number
 * @direct_blocks: Direct block pointers (up to MYFS_DIRECT_BLOCKS_MAX)
 * @block_count: Number of blocks allocated to this inode
 * @size: File/directory size in bytes
 * @mode: File type (S_IFREG, S_IFDIR, S_IFLNK, etc.)
 * @ref_count: In-memory reference count
 * @links_count: Hard link count
 * @uid: Owner user ID
 * @gid: Owner group ID
 * @permissions: File permissions (e.g., 0755)
 * @atime: Access time (Unix timestamp)
 * @mtime: Modification time (Unix timestamp)
 * @ctime: Creation time (Unix timestamp)
 * @dirty: Flag indicating inode needs to be written to disk
 *
 * This structure represents an in-memory inode with no VFS dependencies.
 */
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

/**
 * struct MyfsDiskSuperBlock - On-disk superblock structure
 * @magic: Magic number to identify the filesystem (MYFS_MAGIC)
 * @total_inodes: Total number of inodes in the filesystem
 * @total_blocks: Total number of blocks in the filesystem
 * @free_inodes: Number of free inodes (currently unused)
 * @free_blocks: Number of free blocks (currently unused)
 * @block_sectors: Number of sectors per block
 * @file_initial_blocks: Initial number of blocks allocated for new files
 * @inode_bitmap_start: First block of inode bitmap
 * @block_bitmap_start: First block of block bitmap
 * @inode_table_start: First block of inode table
 * @data_blocks_start: First block of data area
 * @root_inode: Root directory inode number
 */
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
} MyfsDiskSuperBlock;

#define INODE_HASH_SIZE 256
#define INODE_HASH(ino) ((ino) % INODE_HASH_SIZE)

/**
 * struct inode_hash_entry - Inode cache hash table entry
 * @inode: Pointer to cached inode
 * @next: Next entry in hash bucket (chaining for collisions)
 */
typedef struct inode_hash_entry {
  MyfsInode *inode;
  struct inode_hash_entry *next;
} InodeHashEntry;

/**
 * struct MyfsSuperBlock - In-memory superblock structure
 * @on_disk: On-disk superblock data
 * @block_bitmap: In-memory block allocation bitmap
 * @inode_bitmap: In-memory inode allocation bitmap
 * @inode_hash_table: Hash table for inode cache
 * @mounted: Flag indicating if filesystem is mounted
 * @block_bytes: Block size in bytes (computed from block_sectors)
 */
typedef struct {
  MyfsDiskSuperBlock on_disk;
  uint8_t *block_bitmap;
  uint8_t *inode_bitmap;
  InodeHashEntry *inode_hash_table[INODE_HASH_SIZE];
  int mounted;
  uint32_t block_bytes;
} MyfsSuperBlock;

/**
 * myfs_format() - Format a disk with MyFS filesystem
 *
 * Creates the superblock, bitmaps, and inode table structures.
 *
 * Return: 0 on success, negative on error
 */
int myfs_format(void);

/**
 * myfs_sb_read() - Read superblock from disk
 *
 * Loads the superblock and initializes the in-memory structures.
 *
 * Return: Pointer to superblock on success, NULL on error
 */
MyfsSuperBlock *myfs_sb_read(void);

/**
 * myfs_sb_kill() - Unmount filesystem and free superblock
 * @sb: Superblock to destroy
 *
 * Flushes all dirty data and frees memory.
 *
 * Return: 0 on success
 */
int myfs_sb_kill(MyfsSuperBlock *sb);

/**
 * myfs_rename() - Rename/move a file or directory
 * @sb: Superblock
 * @old_parent_inode: Old parent directory inode
 * @old_name: Current name
 * @new_parent_inode: New parent directory inode
 * @new_name: New name
 *
 * Return: 0 on success, negative on error
 */
int myfs_rename(MyfsSuperBlock *sb, MyfsInode *old_parent_inode,
                const char *old_name, MyfsInode *new_parent_inode,
                const char *new_name);

/**
 * myfs_read_symlink() - Read the target path of a symbolic link
 * @sb: Superblock
 * @inode: Symlink inode
 * @buf: Buffer to store target path
 * @bufsize: Size of buffer
 *
 * Return: Number of bytes read, or negative on error
 */
ssize_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsInode *inode, char *buf,
                          size_t bufsize);

/**
 * myfs_create_symlink() - Create a symbolic link
 * @sb: Superblock
 * @dir_inode: Parent directory inode
 * @name: Name of the symlink
 * @target: Target path the symlink points to
 *
 * Return: 0 on success, negative on error
 */
int myfs_create_symlink(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                        const char *name, const char *target);

/**
 * myfs_readdir() - Read a directory entry at a given offset
 * @sb: Superblock
 * @dir_inode: Directory inode
 * @offset: Byte offset into directory
 * @entry: Pointer to store directory entry
 *
 * Return: Number of bytes read, 0 at end of directory, negative on error
 */
int myfs_readdir(MyfsSuperBlock *sb, MyfsInode *dir_inode, uint32_t offset,
                 MyfsDirEntry *entry);

/**
 * myfs_disk_inode_read() - Read an inode from disk
 * @sb: Superblock
 * @ino: Inode number to read
 *
 * Return: Pointer to newly allocated inode, or NULL on error
 */
MyfsInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino);

/**
 * myfs_disk_inode_write() - Write an inode to disk
 * @sb: Superblock
 * @ino: Inode number to write
 *
 * Return: 0 on success, negative on error
 */
int myfs_disk_inode_write(MyfsSuperBlock *sb, int ino);

/**
 * myfs_iget() - Get an inode (from cache or disk)
 * @sb: Superblock
 * @ino: Inode number
 *
 * Increments the reference count. Must be balanced with myfs_iput().
 *
 * Return: Pointer to inode, or NULL on error
 */
MyfsInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino);

/**
 * myfs_iput() - Release an inode reference
 * @sb: Superblock
 * @inode: Inode to release
 *
 * Decrements reference count and writes back if dirty and ref_count reaches 0.
 */
void myfs_iput(MyfsSuperBlock *sb, MyfsInode *inode);

/**
 * myfs_inode_alloc() - Allocate a new inode
 * @sb: Superblock
 * @inode: Pointer to store newly allocated inode
 * @mode: File mode (S_IFREG, S_IFDIR, S_IFLNK, etc.)
 *
 * Return: 0 on success, negative on error
 */
int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsInode **inode, int mode);

/**
 * myfs_read_inode() - Read data from an inode
 * @sb: Superblock
 * @inode: Inode to read from
 * @offset: Byte offset to start reading
 * @buf: Buffer to store read data
 * @count: Number of bytes to read
 *
 * Return: Number of bytes read, or negative on error
 */
ssize_t myfs_node_read(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                       void *buf, size_t count);

/**
 * myfs_write_inode() - Write data to an inode
 * @sb: Superblock
 * @inode: Inode to write to
 * @offset: Byte offset to start writing
 * @buf: Buffer containing data to write
 * @count: Number of bytes to write
 *
 * Return: Number of bytes written, or negative on error
 */
ssize_t myfs_node_write(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                        const void *buf, size_t count);

/**
 * myfs_truncate_inode() - Change the size of an inode
 * @sb: Superblock
 * @inode: Inode to truncate
 * @new_size: New size in bytes
 *
 * Frees blocks if shrinking the file.
 *
 * Return: 0 on success, negative on error
 */
int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsInode *inode,
                        uint32_t new_size);

/**
 * myfs_lookup() - Look up a directory entry by name
 * @sb: Superblock
 * @dir_inode: Directory inode to search in
 * @name: Name to search for
 * @found_ino: Pointer to store found inode number
 *
 * Return: 0 on success, negative if not found or on error
 */
int myfs_lookup(MyfsSuperBlock *sb, MyfsInode *dir_inode, const char *name,
                uint32_t *found_ino);

/**
 * myfs_add_dir_entry() - Add an entry to a directory
 * @sb: Superblock
 * @dir_inode: Directory inode
 * @name: Name of the new entry
 * @inode_num: Inode number of the new entry
 *
 * Return: 0 on success, negative on error
 */
int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                       const char *name, uint32_t inode_num);

/**
 * myfs_remove_dir_entry() - Remove an entry from a directory
 * @sb: Superblock
 * @dir_inode: Directory inode
 * @name: Name of entry to remove
 *
 * Return: 0 on success, negative on error
 */
int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                      const char *name);

/**
 * myfs_create_file() - Create a new file
 * @sb: Superblock
 * @parent_dir: Parent directory inode
 * @name: Name of the new file
 * @new_ino: Pointer to store new inode number
 *
 * Return: 0 on success, negative on error
 */
int myfs_create_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                     const char *name, uint32_t *new_ino);

/**
 * myfs_create_dir() - Create a new directory
 * @sb: Superblock
 * @parent_dir: Parent directory inode
 * @name: Name of the new directory
 * @new_ino: Pointer to store new inode number
 *
 * Automatically creates "." and ".." entries.
 *
 * Return: 0 on success, negative on error
 */
int myfs_create_dir(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name,
                    uint32_t *new_ino);

/**
 * myfs_unlink() - Delete a file or directory
 * @sb: Superblock
 * @parent_dir: Parent directory inode
 * @name: Name of entry to delete
 *
 * Return: 0 on success, negative on error
 */
int myfs_unlink(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name);

/**
 * myfs_find_entry_idx() - Find the index of a directory entry
 * @entries: Array of directory entries
 * @entries_count: Number of entries in array
 * @name: Name to search for
 *
 * Return: Index of entry, or -1 if not found
 */
int myfs_entry_idx_find(MyfsDirEntry *entries, int entries_count,
                        const char *name);

/**
 * myfs_get_version() - Get filesystem version string
 *
 * Return: Version string
 */
const char *myfs_version_get(void);

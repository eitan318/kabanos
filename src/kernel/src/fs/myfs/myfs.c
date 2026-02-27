#include "fs/myfs/myfs.h"
#include "drivers/block/blockdev.h"
#include "klib/stdbool.h"
#include "klib/stdint.h"
#include "klib/string.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "utils/math.h"

static void disk_read_block(uint32_t block_addr, void *buffer,
                            int block_sectors) {
  ata_read_sector(block_addr * block_sectors, block_sectors, buffer);
}

static void disk_write_block(uint32_t block_addr, const void *buffer,
                             int block_sectors) {
  ata_write_sector(block_addr * block_sectors, block_sectors, buffer);
}

static void disk_read_bitmap(uint32_t block_addr, void *buffer,
                             int bytes_to_read, int block_bytes,
                             int block_sectors) {
  uint8_t *buf = buffer;
  int full_blocks = bytes_to_read / block_bytes;
  int remainder = bytes_to_read % block_bytes;

  for (int i = 0; i < full_blocks; i++) {
    ata_read_sector((block_addr + i) * block_sectors, block_sectors,
                    buf + i * block_bytes);
  }

  if (remainder > 0) {
    uint8_t tmp[block_bytes];
    ata_read_sector((block_addr + full_blocks) * block_sectors, block_sectors,
                    tmp);
    memcpy(buf + full_blocks * block_bytes, tmp, remainder);
  }
}

static void disk_write_bitmap(uint32_t block_addr, const void *buffer,
                              int buffer_bytes, int block_sectors) {
  const int block_bytes = block_sectors * SECTOR_BYTES;
  const uint8_t *buf = buffer;
  int full_blocks = buffer_bytes / block_bytes;
  int remainder = buffer_bytes % block_bytes;

  for (int i = 0; i < full_blocks; i++) {
    ata_write_sector((block_addr + i) * block_sectors, block_sectors,
                     buf + i * block_bytes);
  }

  if (remainder > 0) {
    uint8_t tmp[block_bytes];
    memset(tmp, 0, block_bytes);
    memcpy(tmp, buf + full_blocks * block_bytes, remainder);
    ata_write_sector((block_addr + full_blocks) * block_sectors, block_sectors,
                     tmp);
  }
}

static int find_first_empty_bit(uint8_t *bitmap, size_t size_in_bytes,
                                size_t start_bit) {
  size_t byte_start = start_bit / 8;
  int bit_start = start_bit % 8;

  for (size_t byte = byte_start; byte < size_in_bytes; byte++) {
    int b_start = (byte == byte_start) ? bit_start : 0;

    if (bitmap[byte] != 0xFF) {
      for (int bit = b_start; bit < 8; bit++) {
        if ((bitmap[byte] & (1 << bit)) == 0) {
          return (int)(byte * 8 + bit);
        }
      }
    }
  }
  return -1;
}

static void bitmap_set(uint8_t bitmap[], int num) {
  bitmap[num / 8] |= (1 << (num % 8));
}

static void bitmap_clear(uint8_t bitmap[], int num) {
  bitmap[num / 8] &= ~(1 << (num % 8));
}

bool bitmap_is_set(uint8_t bitmap[], int num) {
  return bitmap[num / 8] & (1 << (num % 8));
}

static MyfsInode *myfs_cache_find(InodeHashEntry **inode_hash_table,
                                  uint32_t ino) {
  int hash = INODE_HASH(ino);
  for (InodeHashEntry *entry = inode_hash_table[hash]; entry;
       entry = entry->next) {
    if (entry->inode->i_ino == ino) {
      return entry->inode;
    }
  }
  return NULL;
}

static void myfs_inode_cache_add(InodeHashEntry **inode_hash_table,
                                 MyfsInode *inode) {
  int hash = INODE_HASH(inode->i_ino);
  InodeHashEntry *entry = kmalloc(sizeof(InodeHashEntry));
  if (!entry)
    return;

  entry->inode = inode;
  entry->next = inode_hash_table[hash];
  inode_hash_table[hash] = entry;
}

static void myfs_inode_cache_remove(InodeHashEntry **inode_hash_table,
                                    uint32_t ino) {
  int hash = INODE_HASH(ino);
  InodeHashEntry **entry = &inode_hash_table[hash];

  while (*entry) {
    if ((*entry)->inode->i_ino == ino) {
      InodeHashEntry *to_remove = *entry;
      *entry = (*entry)->next;
      MyfsInode *inode = to_remove->inode;
      kfree(to_remove);
      kfree(inode);
      return;
    }
    entry = &(*entry)->next;
  }
}

static int disk_rw_spanning(MyfsSuperBlock *sb, uint32_t start_block,
                            uint32_t offset_in_first_block, void *data,
                            size_t data_size, bool is_write) {
  uint8_t *ptr = data;
  size_t remaining = data_size;
  uint32_t cur_block = start_block;
  uint32_t offset = offset_in_first_block;

  while (remaining > 0) {
    uint8_t block_buf[sb->block_bytes];

    if (is_write) {
      if (offset != 0 || remaining < sb->block_bytes) {
        disk_read_block(cur_block, block_buf, sb->on_disk.block_sectors);
      }
    } else {
      disk_read_block(cur_block, block_buf, sb->on_disk.block_sectors);
    }

    size_t chunk = min_int(sb->block_bytes - offset, remaining);

    if (is_write) {
      memcpy(block_buf + offset, ptr, chunk);
      disk_write_block(cur_block, block_buf, sb->on_disk.block_sectors);
    } else {
      memcpy(ptr, block_buf + offset, chunk);
    }

    ptr += chunk;
    remaining -= chunk;
    cur_block++;
    offset = 0;
  }
  return 0;
}

MyfsInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino) {
  if (!sb)
    return NULL;

  MyfsInode *inode = kmalloc(sizeof(*inode));
  if (!inode)
    return NULL;

  int inode_size = sizeof(*inode);
  uint64_t inode_offset_bytes = (uint64_t)ino * (uint64_t)inode_size;
  uint64_t first_block_index;
  uint32_t offset_in_block;
  div64_32(inode_offset_bytes, sb->block_bytes, &first_block_index,
           &offset_in_block);
  uint32_t start_block = sb->on_disk.inode_table_start + first_block_index;

  disk_rw_spanning(sb, start_block, offset_in_block, inode, inode_size, false);
  return inode;
}

int myfs_disk_inode_write(MyfsSuperBlock *sb, int ino) {
  if (!sb)
    return -1;

  MyfsInode *inode = myfs_iget(sb, ino);
  if (!inode)
    return -1;

  if (!inode->dirty) {
    myfs_iput(sb, inode);
    return 0;
  }

  int inode_size = sizeof(*inode);
  uint64_t inode_offset_bytes = (uint64_t)inode->i_ino * (uint64_t)inode_size;
  uint64_t first_block_index;
  uint32_t offset_in_block;
  div64_32(inode_offset_bytes, sb->block_bytes, &first_block_index,
           &offset_in_block);
  uint32_t start_block = sb->on_disk.inode_table_start + first_block_index;

  disk_rw_spanning(sb, start_block, offset_in_block, inode, inode_size, true);
  inode->dirty = 0;
  myfs_iput(sb, inode);
  return 0;
}

static void inodes_flush(MyfsSuperBlock *sb) {
  if (!sb)
    return;

  for (int i = 0; i < INODE_HASH_SIZE; i++) {
    InodeHashEntry *inode_hash_entry = sb->inode_hash_table[i];
    if (inode_hash_entry && inode_hash_entry->inode->dirty) {
      myfs_disk_inode_write(sb, inode_hash_entry->inode->i_ino);
    }
  }
}

int myfs_format() {
  enum {
    MYFS_MAX_INODES = 1024,
    MYFS_MAX_BLOCKS = 4096,
    MYFS_BLOCK_SECTORS = 4,
    MYFS_FILE_INIT_BLOCK_COUNT = 1
  };

  const int MYFS_BLOCK_BYTES = MYFS_BLOCK_SECTORS * SECTOR_SIZE;
  uint32_t total_blocks = MYFS_MAX_BLOCKS;
  uint32_t total_inodes = total_blocks / 4;

  uint32_t block_bitmap_start = 1;
  uint32_t inode_bitmap_start =
      block_bitmap_start +
      (total_blocks + MYFS_BLOCK_BYTES - 1) / MYFS_BLOCK_BYTES;
  uint32_t inode_table_start =
      inode_bitmap_start +
      (total_inodes + MYFS_BLOCK_BYTES - 1) / MYFS_BLOCK_BYTES;
  uint32_t data_blocks_start =
      inode_table_start +
      (total_inodes * sizeof(MyfsInode) + MYFS_BLOCK_BYTES - 1) /
          MYFS_BLOCK_BYTES;

  MyfsDiskSuperBlock sb = {.magic = MYFS_MAGIC,
                           .total_inodes = total_inodes,
                           .total_blocks = total_blocks,
                           .block_sectors = MYFS_BLOCK_SECTORS,
                           .root_inode = MYFS_ROOT_INODE_NUM,
                           .block_bitmap_start = block_bitmap_start,
                           .inode_bitmap_start = inode_bitmap_start,
                           .inode_table_start = inode_table_start,
                           .file_initial_blocks = MYFS_FILE_INIT_BLOCK_COUNT,
                           .data_blocks_start = data_blocks_start};

  disk_write_block(0, &sb, sb.block_sectors);

  uint32_t inode_bitmap_bytes = (total_inodes + 7) / 8;
  uint32_t block_bitmap_bytes = (total_blocks + 7) / 8;

  uint8_t *inode_bitmap = kmalloc(inode_bitmap_bytes);
  uint8_t *block_bitmap = kmalloc(block_bitmap_bytes);
  if (!inode_bitmap || !block_bitmap) {
    kfree(inode_bitmap);
    kfree(block_bitmap);
    return -1;
  }

  memset(inode_bitmap, 0, inode_bitmap_bytes);
  memset(block_bitmap, 0, block_bitmap_bytes);

  for (int i = 0; i < sb.data_blocks_start; i++) {
    bitmap_set(block_bitmap, i);
  }

  disk_write_bitmap(sb.block_bitmap_start, block_bitmap, block_bitmap_bytes,
                    sb.block_sectors);
  disk_write_bitmap(sb.inode_bitmap_start, inode_bitmap, inode_bitmap_bytes,
                    sb.block_sectors);

  return 0;
}

MyfsSuperBlock *myfs_sb_read() {
  MyfsSuperBlock *sb = kmalloc(sizeof(*sb));
  if (!sb)
    return NULL;

  memset(sb->inode_hash_table, 0, sizeof(sb->inode_hash_table));

  uint32_t sectors_to_read =
      (sizeof(sb->on_disk) + SECTOR_BYTES - 1) / SECTOR_BYTES;
  uint8_t tmp_buf[sectors_to_read * SECTOR_BYTES];

  ata_read_sector(0, sectors_to_read, tmp_buf);
  memcpy(&sb->on_disk, tmp_buf, sizeof(sb->on_disk));

  sb->block_bytes = sb->on_disk.block_sectors * SECTOR_BYTES;

  if (sb->on_disk.magic != MYFS_MAGIC) {
    kfree(sb);
    return NULL;
  }

  int block_bitmap_bytes =
      (sb->on_disk.total_blocks + (sizeof(*sb->block_bitmap) * 8) - 1) /
      (sizeof(*sb->block_bitmap) * 8);
  int inode_bitmap_bytes =
      (sb->on_disk.total_inodes + (sizeof(*sb->inode_bitmap) * 8) - 1) /
      (sizeof(*sb->inode_bitmap) * 8);

  sb->block_bitmap = kmalloc(block_bitmap_bytes);
  sb->inode_bitmap = kmalloc(inode_bitmap_bytes);

  disk_read_bitmap(sb->on_disk.block_bitmap_start, sb->block_bitmap,
                   block_bitmap_bytes, sb->block_bytes,
                   sb->on_disk.block_sectors);
  disk_read_bitmap(sb->on_disk.inode_bitmap_start, sb->inode_bitmap,
                   inode_bitmap_bytes, sb->block_bytes,
                   sb->on_disk.block_sectors);

  sb->mounted = 1;
  return sb;
}

int myfs_sb_kill(MyfsSuperBlock *sb) {
  if (sb) {
    inodes_flush(sb);

    int block_bitmap_bytes = (sb->on_disk.total_blocks + 7) / 8;
    int inode_bitmap_bytes = (sb->on_disk.total_inodes + 7) / 8;

    disk_write_bitmap(sb->on_disk.block_bitmap_start, sb->block_bitmap,
                      block_bitmap_bytes, sb->on_disk.block_sectors);
    disk_write_bitmap(sb->on_disk.inode_bitmap_start, sb->inode_bitmap,
                      inode_bitmap_bytes, sb->on_disk.block_sectors);

    sb->mounted = 0;
    if (sb->block_bitmap)
      kfree(sb->block_bitmap);
    if (sb->inode_bitmap)
      kfree(sb->inode_bitmap);
    kfree(sb);
  }
  return 0;
}

MyfsInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino) {
  if (!bitmap_is_set(sb->inode_bitmap, ino)) {
    return NULL;
  }

  MyfsInode *inode = myfs_cache_find(sb->inode_hash_table, ino);
  if (inode) {
    inode->ref_count++;
    return inode;
  }

  inode = myfs_disk_inode_read(sb, ino);
  if (inode) {
    inode->ref_count = 1;
    inode->dirty = 1;
    myfs_inode_cache_add(sb->inode_hash_table, inode);
  }
  return inode;
}

void myfs_iput(MyfsSuperBlock *sb, MyfsInode *inode) {
  if (!inode)
    return;

  inode->ref_count--;
  if (inode->ref_count <= 0) {
    if (inode->dirty) {
      myfs_disk_inode_write(sb, inode->i_ino);
    }
    myfs_inode_cache_remove(sb->inode_hash_table, inode->i_ino);
  }
}

static int myfs_blocks_alloc(MyfsSuperBlock *sb, uint32_t *block_array,
                             int blocks_amount_alloc) {
  if (!block_array)
    return -1;

  for (int i = 0; i < blocks_amount_alloc; i++) {
    int block_num = find_first_empty_bit(sb->block_bitmap,
                                         (sb->on_disk.total_blocks + 8 - 1) / 8,
                                         sb->on_disk.data_blocks_start);
    if (block_num == -1) {
      for (int j = 0; j < i; j++) {
        bitmap_clear(sb->block_bitmap, block_array[j]);
      }
      return -1;
    }
    block_array[i] = block_num;
    bitmap_set(sb->block_bitmap, block_num);
  }
  return 0;
}

static void myfs_blocks_free(MyfsSuperBlock *sb, uint32_t *block_list,
                             int blocks_amount) {
  if (!block_list)
    return;

  for (int i = 0; i < blocks_amount; i++) {
    bitmap_clear(sb->block_bitmap, block_list[i]);
  }
}

int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsInode **inode, int mode) {
  int inode_num = find_first_empty_bit(
      sb->inode_bitmap, (sb->on_disk.total_inodes + 8 - 1) / 8, 0);
  if (inode_num == -1)
    return -1;

  bitmap_set(sb->inode_bitmap, inode_num);

  size_t bitmap_size_bytes = (sb->on_disk.total_inodes + 7) / 8;
  uint32_t bitmap_block_idx = inode_num / (sb->block_bytes * 8);
  int bytes_to_write = min_int(
      bitmap_size_bytes - bitmap_block_idx * sb->block_bytes, sb->block_bytes);

  disk_write_bitmap(sb->on_disk.inode_bitmap_start + bitmap_block_idx,
                    sb->inode_bitmap + bitmap_block_idx * sb->block_bytes,
                    bytes_to_write, sb->on_disk.block_sectors);

  MyfsInode *new_inode = kmalloc(sizeof(*new_inode));
  memset(new_inode, 0, sizeof(*new_inode));

  new_inode->i_ino = inode_num;
  new_inode->size = 0;
  new_inode->mode = mode;

  if (myfs_blocks_alloc(sb, new_inode->direct_blocks,
                        sb->on_disk.file_initial_blocks) < 0) {
    bitmap_clear(sb->inode_bitmap, inode_num);
    kfree(new_inode);
    return -1;
  }

  if (S_ISREG(mode)) {
    new_inode->block_count = sb->on_disk.file_initial_blocks;
  } else if (S_ISDIR(mode)) {
    new_inode->block_count = 1;
  } else if (S_ISLNK(mode)) {
    new_inode->block_count = 0;
  } else {
    new_inode->block_count = 1;
  }

  new_inode->ref_count = 1;
  new_inode->links_count = 1;
  new_inode->permissions = !S_ISDIR(mode) ? 0755 : 0644;
  new_inode->dirty = 1;

  uint64_t now = 5554; //(uint64_t)time(NULL);
  new_inode->atime = now;
  new_inode->mtime = now;
  new_inode->ctime = now;

  *inode = new_inode;
  myfs_inode_cache_add(sb->inode_hash_table, *inode);
  myfs_disk_inode_write(sb, new_inode->i_ino);
  return 0;
}

static void myfs_inode_free(MyfsSuperBlock *sb, MyfsInode *inode) {
  if (!inode)
    return;

  bitmap_clear(sb->inode_bitmap, inode->i_ino);
  myfs_blocks_free(sb, inode->direct_blocks, inode->block_count);
  myfs_inode_cache_remove(sb->inode_hash_table, inode->i_ino);
}

int myfs_rename(MyfsSuperBlock *sb, MyfsInode *old_parent_inode,
                const char *old_name, MyfsInode *new_parent_inode,
                const char *new_name) {
  uint32_t found_ino = 0;
  if (myfs_lookup(sb, old_parent_inode, old_name, &found_ino) == -1) {
    return -1;
  }

  if (myfs_dir_rm_entry(sb, old_parent_inode, old_name) == -1) {
    return -1;
  }

  myfs_unlink(sb, new_parent_inode, new_name);

  if (myfs_dir_add_entry(sb, new_parent_inode, new_name, found_ino) == -1) {
    return -1;
  }
  return 0;
}

ssize_t myfs_node_read(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                       void *buf, size_t count) {
  if (!inode || offset >= inode->size)
    return 0;

  if (offset + count > inode->size) {
    count = inode->size - offset;
  }

  uint32_t bytes_read = 0;
  uint8_t *out = (uint8_t *)buf;
  uint8_t block_buf[sb->block_bytes];

  while (bytes_read < count) {
    uint32_t block_idx = offset / sb->block_bytes;
    uint32_t block_offset = offset % sb->block_bytes;
    uint32_t to_read =
        min_int(sb->block_bytes - block_offset, count - bytes_read);

    if (block_idx >= inode->block_count)
      break;

    disk_read_block(inode->direct_blocks[block_idx], block_buf,
                    sb->on_disk.block_sectors);
    memcpy(out + bytes_read, block_buf + block_offset, to_read);

    offset += to_read;
    bytes_read += to_read;
  }

  inode->atime = 5554; //(uint64_t)time(NULL);
  inode->dirty = 1;
  return bytes_read;
}

ssize_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsInode *inode, char *buf,
                          size_t bufsize) {
  if (!S_ISLNK(inode->mode)) {
    return -1;
  }
  size_t len = min_int(inode->size, bufsize);
  return myfs_node_read(sb, inode, 0, buf, len);
}

int myfs_create_symlink(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                        const char *name, const char *target) {
  uint32_t existing_ino;
  if (myfs_lookup(sb, parent_dir, name, &existing_ino) == 0) {
    return -1;
  }

  MyfsInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFLNK) < 0) {
    return -1;
  }

  int target_len = strlen(target);
  if (myfs_node_write(sb, new_inode, 0, target, target_len) < 0) {
    myfs_inode_free(sb, new_inode);
    return -1;
  }

  if (myfs_dir_add_entry(sb, parent_dir, name, new_inode->i_ino) < 0) {
    myfs_inode_free(sb, new_inode);
    return -1;
  }

  myfs_iput(sb, new_inode);
  return 0;
}

ssize_t myfs_node_write(MyfsSuperBlock *sb, MyfsInode *inode, uint32_t offset,
                        const void *buf, size_t count) {
  if (!inode || !buf || count == 0)
    return 0;

  int block_bytes = sb->block_bytes;
  int target_blocks = (offset + count + block_bytes - 1) / block_bytes;

  if (target_blocks > MYFS_DIRECT_BLOCKS_MAX) {
    return -1;
  }

  if (target_blocks > inode->block_count) {
    int additional_blocks = target_blocks - inode->block_count;
    myfs_blocks_alloc(sb, &inode->direct_blocks[inode->block_count],
                      additional_blocks);
    inode->block_count += additional_blocks;
  }

  uint32_t bytes_written = 0;
  const uint8_t *in = (const uint8_t *)buf;
  uint8_t block_buf[block_bytes];

  while (bytes_written < count) {
    uint32_t block_idx = offset / block_bytes;
    uint32_t block_offset = offset % block_bytes;
    uint32_t to_write =
        min_int(block_bytes - block_offset, count - bytes_written);

    if (to_write < block_bytes) {
      disk_read_block(inode->direct_blocks[block_idx], block_buf,
                      sb->on_disk.block_sectors);
    }

    memcpy(block_buf + block_offset, in + bytes_written, to_write);
    disk_write_block(inode->direct_blocks[block_idx], block_buf,
                     sb->on_disk.block_sectors);

    offset += to_write;
    bytes_written += to_write;
  }

  if (offset > inode->size) {
    inode->size = offset;
  }

  inode->mtime = 5554; //(uint64_t)time(NULL);
  inode->dirty = 1;
  myfs_disk_inode_write(sb, inode->i_ino);
  return bytes_written;
}

int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsInode *inode,
                        uint32_t new_size) {
  if (!inode)
    return -1;

  int block_bytes = sb->block_bytes;

  if (new_size < inode->size) {
    int new_blocks = (new_size + block_bytes - 1) / block_bytes;
    if (new_blocks < inode->block_count) {
      for (int i = new_blocks; i < inode->block_count; i++) {
        bitmap_clear(sb->block_bitmap, inode->direct_blocks[i]);
      }
      5554;
      inode->block_count = new_blocks;
    }
  }

  inode->size = new_size;
  inode->mtime = 5554; //(uint64_t)time(NULL);
  inode->dirty = 1;
  return 0;
}

int myfs_entry_idx_find(MyfsDirEntry *entries, int entries_count,
                        const char *name) {
  for (int i = 0; i < entries_count; i++) {
    if (strcmp(entries[i].file_name, name) == 0) {
      return i;
    }
  }
  return -1;
}

int myfs_lookup(MyfsSuperBlock *sb, MyfsInode *dir_inode, const char *name,
                uint32_t *found_ino) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode)) {
    return -1;
  }

  int entries_count = dir_inode->size / sizeof(MyfsDirEntry);
  MyfsDirEntry *entries = kmalloc(dir_inode->size);
  if (!entries)
    return -1;

  if (myfs_node_read(sb, dir_inode, 0, entries, dir_inode->size) < 0) {
    kfree(entries);
    return -1;
  }

  int target_idx = myfs_entry_idx_find(entries, entries_count, name);
  if (target_idx == -1) {
    kfree(entries);
    return -1;
  }

  *found_ino = entries[target_idx].inode_num;
  kfree(entries);
  return 0;
}

int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                       const char *name, uint32_t inode_num) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode)) {
    return -1;
  }

  uint32_t found_ino = 0;
  if (myfs_lookup(sb, dir_inode, name, &found_ino) == 0) {
    return -1;
  }

  MyfsDirEntry new_entry;
  new_entry.inode_num = inode_num;
  strncpy(new_entry.file_name, name, sizeof(new_entry.file_name) - 1);
  new_entry.file_name[sizeof(new_entry.file_name) - 1] = '\0';

  int res = myfs_node_write(sb, dir_inode, dir_inode->size, &new_entry,
                            sizeof(new_entry));
  return res;
}

int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsInode *dir_inode,
                      const char *name) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode)) {
    return -1;
  }

  int entries_count = dir_inode->size / sizeof(MyfsDirEntry);
  MyfsDirEntry *entries = kmalloc(dir_inode->size);
  if (!entries)
    return -1;

  if (myfs_node_read(sb, dir_inode, 0, entries, dir_inode->size) < 0) {
    kfree(entries);
    return -1;
  }

  int target_idx = myfs_entry_idx_find(entries, entries_count, name);
  if (target_idx == -1) {
    kfree(entries);
    return -1;
  }

  for (int i = target_idx; i < entries_count - 1; i++) {
    entries[i] = entries[i + 1];
  }

  int new_size = (entries_count - 1) * sizeof(MyfsDirEntry);
  myfs_node_write(sb, dir_inode, 0, entries, new_size);
  myfs_inode_truncate(sb, dir_inode, new_size);

  kfree(entries);
  return 0;
}

int myfs_readdir(MyfsSuperBlock *sb, MyfsInode *dir_inode, uint32_t offset,
                 MyfsDirEntry *entry) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode) || !entry) {
    return -1;
  }

  if (offset >= dir_inode->size) {
    return 0;
  }

  return myfs_node_read(sb, dir_inode, offset, entry, sizeof(MyfsDirEntry));
}

int myfs_create_file(MyfsSuperBlock *sb, MyfsInode *parent_dir,
                     const char *name, uint32_t *new_ino) {
  uint32_t existing_ino;
  if (myfs_lookup(sb, parent_dir, name, &existing_ino) == 0) {
    return -1;
  }

  MyfsInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFREG) < 0) {
    return -1;
  }

  if (myfs_dir_add_entry(sb, parent_dir, name, new_inode->i_ino) < 0) {
    myfs_inode_free(sb, new_inode);
    return -1;
  }

  *new_ino = new_inode->i_ino;
  myfs_iput(sb, new_inode);
  return 0;
}

int myfs_create_dir(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name,
                    uint32_t *new_ino) {
  uint32_t existing_ino;
  if (myfs_lookup(sb, parent_dir, name, &existing_ino) == 0) {
    return -1;
  }

  MyfsInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFDIR) < 0) {
    return -1;
  }

  if (myfs_dir_add_entry(sb, parent_dir, name, new_inode->i_ino) < 0) {
    myfs_inode_free(sb, new_inode);
    return -1;
  }

  myfs_dir_add_entry(sb, new_inode, ".", new_inode->i_ino);
  myfs_dir_add_entry(sb, new_inode, "..", parent_dir->i_ino);

  *new_ino = new_inode->i_ino;
  myfs_iput(sb, new_inode);
  return 0;
}

int myfs_unlink(MyfsSuperBlock *sb, MyfsInode *parent_dir, const char *name) {
  uint32_t target_ino;
  if (myfs_lookup(sb, parent_dir, name, &target_ino) < 0) {
    return -1;
  }

  MyfsInode *target_inode = myfs_iget(sb, target_ino);
  if (target_inode) {
    myfs_inode_free(sb, target_inode);
  }

  return myfs_dir_rm_entry(sb, parent_dir, name);
}

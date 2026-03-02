#include "fs/myfs/myfs.h"

#define MYFS_BLOCK_SECTORS 4
#define MYFS_BLOCK_BYTES (MYFS_BLOCK_SECTORS * SECTOR_BYTES)

static void log_bitmap_hex(MyfsSuperBlock *sb, const uint8_t *buf, int bytes) {
  for (int i = 0; i < bytes && i < 32; i++) { // cap at 32 bytes
    sb->plt->log("%x ", buf[i]);
  }
  sb->plt->log("\n");
}

static void disk_read_bitmap(MyfsSuperBlock *sb, uint32_t block_addr,
                             void *buffer, int bytes_to_read) {
  uint8_t *buf = buffer;
  int full_blocks = bytes_to_read / sb->block_bytes;
  int remainder = bytes_to_read % sb->block_bytes;
  for (int i = 0; i < full_blocks; i++) {
    sb->plt->read_block(sb->dev, block_addr + i, buf + i * sb->block_bytes);
  }
  if (remainder > 0) {
    uint8_t tmp[sb->block_bytes];
    sb->plt->read_block(sb->dev, block_addr + full_blocks, tmp);
    memcpy(buf + full_blocks * sb->block_bytes, tmp, remainder);
  }
  sb->plt->log("Read bitmap block=%u bytes=%d: ", block_addr, bytes_to_read);
  log_bitmap_hex(sb, buf, bytes_to_read);
}

static void disk_write_bitmap(MyfsSuperBlock *sb, uint32_t block_addr,
                              const void *buffer, int buffer_bytes) {
  sb->plt->log("Write bitmap block=%u bytes=%d: ", block_addr, buffer_bytes);
  log_bitmap_hex(sb, (const uint8_t *)buffer, buffer_bytes);
  const uint8_t *buf = buffer;
  int full_blocks = buffer_bytes / sb->block_bytes;
  int remainder = buffer_bytes % sb->block_bytes;
  for (int i = 0; i < full_blocks; i++)
    sb->plt->write_block(sb->dev, block_addr + i, buf + i * sb->block_bytes);
  if (remainder > 0) {
    uint8_t tmp[sb->block_bytes];
    memset(tmp, 0, sb->block_bytes);
    memcpy(tmp, buf + full_blocks * sb->block_bytes, remainder);
    sb->plt->write_block(sb->dev, block_addr + full_blocks, tmp);
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
        if ((bitmap[byte] & (1 << bit)) == 0)
          return (int)(byte * 8 + bit);
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

static MyfsDiskInode *myfs_cache_find(InodeHashEntry **inode_hash_table,
                                      uint32_t ino) {
  int hash = INODE_HASH(ino);
  for (InodeHashEntry *e = inode_hash_table[hash]; e; e = e->next)
    if (e->inode->i_ino == ino)
      return e->inode;
  return NULL;
}

static void myfs_inode_cache_add(MyfsSuperBlock *sb,
                                 InodeHashEntry **inode_hash_table,
                                 MyfsDiskInode *inode) {
  int hash = INODE_HASH(inode->i_ino);
  InodeHashEntry *entry = sb->plt->alloc(sizeof(InodeHashEntry));
  if (!entry)
    return;
  entry->inode = inode;
  entry->next = inode_hash_table[hash];
  inode_hash_table[hash] = entry;
}

static void myfs_inode_cache_remove(MyfsSuperBlock *sb,
                                    InodeHashEntry **inode_hash_table,
                                    uint32_t ino) {
  int hash = INODE_HASH(ino);
  InodeHashEntry **entry = &inode_hash_table[hash];
  while (*entry) {
    if ((*entry)->inode->i_ino == ino) {
      InodeHashEntry *dead = *entry;
      *entry = dead->next;
      sb->plt->free(dead->inode);
      sb->plt->free(dead);
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
    size_t chunk = MIN(sb->block_bytes - offset, remaining);

    if (is_write) {
      /* Read-modify-write when not writing a full block */
      if (offset != 0 || remaining < sb->block_bytes)
        sb->plt->read_block(sb->dev, cur_block, block_buf);
      memcpy(block_buf + offset, ptr, chunk);
      sb->plt->write_block(sb->dev, cur_block, block_buf);
    } else {
      sb->plt->read_block(sb->dev, cur_block, block_buf);
      memcpy(ptr, block_buf + offset, chunk);
    }

    ptr += chunk;
    remaining -= chunk;
    cur_block++;
    offset = 0;
  }
  return 0;
}

MyfsDiskInode *myfs_disk_inode_read(MyfsSuperBlock *sb, int ino) {
  if (!sb)
    return NULL;

  MyfsDiskInode *inode = sb->plt->alloc(sizeof(*inode));
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
  // Don't use myfs_iget here — find directly in cache
  MyfsDiskInode *inode = myfs_cache_find(sb->inode_hash_table, ino);
  if (!inode || !inode->dirty)
    return 0;

  int inode_size = sizeof(*inode);
  uint64_t inode_offset_bytes = (uint64_t)inode->i_ino * (uint64_t)inode_size;
  uint64_t first_block_index;
  uint32_t offset_in_block;
  div64_32(inode_offset_bytes, sb->block_bytes, &first_block_index,
           &offset_in_block);

  uint32_t start_block = sb->on_disk.inode_table_start + first_block_index;
  disk_rw_spanning(sb, start_block, offset_in_block, inode, inode_size, true);
  inode->dirty = 0;
  return 0;
}

static void inodes_flush(MyfsSuperBlock *sb) {
  if (!sb)
    return;
  for (int i = 0; i < INODE_HASH_SIZE; i++) {
    InodeHashEntry *e = sb->inode_hash_table[i];
    if (e && e->inode->dirty)
      myfs_disk_inode_write(sb, e->inode->i_ino);
  }
}

void myfs_print_superblock(const char *label, MyfsDiskSuperBlock *sb,
                           fs_platform_t *plt) {

  plt->log("--- Superblock: %s ---\n", label);
  plt->log("Magic:              0x%x\n", sb->magic);
  plt->log("Total Inodes:       %u\n", sb->total_inodes);
  plt->log("Total Blocks:       %u\n", sb->total_blocks);
  plt->log("Block Sectors:      %u\n", sb->block_sectors);
  plt->log("Inode Table Start:  %u\n", sb->inode_table_start);
  plt->log("Data Blocks Start:  %u\n", sb->data_blocks_start);
  plt->log("Root Inode Num:     %u\n", sb->root_inode);
  plt->log("------------------------------\n");
}

void myfs_print_inode(uint32_t ino_num, MyfsDiskInode *inode,
                      fs_platform_t *plt) {
  plt->log("--- Inode %u ---\n", ino_num);
  plt->log("Mode:   0x%04x\n", inode->mode);
  plt->log("Size:   %u bytes\n", inode->size);
  plt->log("Blocks: ");
  for (int i = 0; i < 8; i++) { // Assuming 8 direct blocks
    if (inode->direct_blocks[i] == 0)
      break;
    plt->log("%u ", inode->direct_blocks[i]);
  }
  plt->log("\n----------------\n");
}

int myfs_format(void *dev, fs_platform_t *plt, int part_blocks) {
  enum {
    MYFS_FILE_INIT_BLOCK_COUNT = 1,
  };

  uint32_t total_blocks = part_blocks;
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
      (total_inodes * sizeof(MyfsDiskInode) + MYFS_BLOCK_BYTES - 1) /
          MYFS_BLOCK_BYTES;

  MyfsDiskSuperBlock on_disk = {
      .magic = MYFS_MAGIC,
      .total_inodes = total_inodes,
      .total_blocks = total_blocks,
      .block_sectors = MYFS_BLOCK_SECTORS,
      .root_inode = MYFS_ROOT_INODE_NUM,
      .block_bitmap_start = block_bitmap_start,
      .inode_bitmap_start = inode_bitmap_start,
      .inode_table_start = inode_table_start,
      .file_initial_blocks = MYFS_FILE_INIT_BLOCK_COUNT,
      .data_blocks_start = data_blocks_start,
  };

  plt->write_block(dev, 0, &on_disk);
  myfs_print_superblock("format", &on_disk, plt);

  MyfsSuperBlock *tmp = plt->alloc(sizeof(*tmp));
  memset(tmp, 0, sizeof(*tmp));
  tmp->dev = dev;
  tmp->on_disk = on_disk;
  tmp->block_bytes = MYFS_BLOCK_BYTES;
  tmp->plt = plt;

  uint32_t inode_bitmap_bytes = (total_inodes + 7) / 8;
  uint32_t block_bitmap_bytes = (total_blocks + 7) / 8;

  uint8_t *inode_bitmap = plt->alloc(inode_bitmap_bytes);
  uint8_t *block_bitmap = plt->alloc(block_bitmap_bytes);

  tmp->inode_bitmap = inode_bitmap;
  tmp->block_bitmap = block_bitmap;

  if (!inode_bitmap || !block_bitmap) {
    plt->free(inode_bitmap);
    plt->free(block_bitmap);
    return -1;
  }

  memset(inode_bitmap, 0, inode_bitmap_bytes);
  memset(block_bitmap, 0, block_bitmap_bytes);
  for (uint32_t i = 0; i < data_blocks_start; i++)
    bitmap_set(block_bitmap, i);

  MyfsDiskInode *root = NULL;
  myfs_inode_alloc(tmp, &root, S_IFDIR | 0755);

  myfs_dir_add_entry(tmp, root, ".", root->i_ino);
  myfs_dir_add_entry(tmp, root, "..", root->i_ino);

  disk_write_bitmap(tmp, block_bitmap_start, block_bitmap, block_bitmap_bytes);
  disk_write_bitmap(tmp, inode_bitmap_start, inode_bitmap, inode_bitmap_bytes);

  myfs_iput(tmp, root);
  myfs_sb_kill(tmp);
  return 0;
}

MyfsSuperBlock *myfs_sb_read(void *dev, fs_platform_t *plt) {
  MyfsSuperBlock *sb = plt->alloc(sizeof(*sb));
  if (!sb)
    return NULL;
  memset(sb, 0, sizeof(*sb));

  sb->plt = plt;
  sb->dev = dev;

  /* read block 0 to get the on-disk superblock */
  uint8_t tmp[MYFS_BLOCK_BYTES];
  plt->read_block(dev, 0, tmp);
  memcpy(&sb->on_disk, tmp, sizeof(sb->on_disk));

  myfs_print_superblock("read", &sb->on_disk, plt);

  if (sb->on_disk.magic != MYFS_MAGIC) {
    sb->plt->free(sb);
    return NULL;
  }

  sb->block_bytes = sb->on_disk.block_sectors * SECTOR_BYTES;

  int block_bitmap_bytes = (sb->on_disk.total_blocks + 7) / 8;
  int inode_bitmap_bytes = (sb->on_disk.total_inodes + 7) / 8;

  sb->block_bitmap = sb->plt->alloc(block_bitmap_bytes);
  sb->inode_bitmap = sb->plt->alloc(inode_bitmap_bytes);
  if (!sb->block_bitmap || !sb->inode_bitmap) {
    sb->plt->free(sb->block_bitmap);
    sb->plt->free(sb->inode_bitmap);
    sb->plt->free(sb);
    return NULL;
  }

  disk_read_bitmap(sb, sb->on_disk.block_bitmap_start, sb->block_bitmap,
                   block_bitmap_bytes);
  disk_read_bitmap(sb, sb->on_disk.inode_bitmap_start, sb->inode_bitmap,
                   inode_bitmap_bytes);

  sb->mounted = 1;
  return sb;
}

int myfs_sb_kill(MyfsSuperBlock *sb) {
  if (!sb)
    return 0;

  sb->plt->log(
      "myfs_sb_kill: flushing bitmaps, inode_bitmap[0]=%x block_bitmap[0]=%x\n",
      sb->inode_bitmap[0], sb->block_bitmap[0]);
  inodes_flush(sb);

  int block_bitmap_bytes = (sb->on_disk.total_blocks + 7) / 8;
  int inode_bitmap_bytes = (sb->on_disk.total_inodes + 7) / 8;

  disk_write_bitmap(sb, sb->on_disk.block_bitmap_start, sb->block_bitmap,
                    block_bitmap_bytes);
  disk_write_bitmap(sb, sb->on_disk.inode_bitmap_start, sb->inode_bitmap,
                    inode_bitmap_bytes);

  sb->mounted = 0;
  sb->plt->free(sb->block_bitmap);
  sb->plt->free(sb->inode_bitmap);
  sb->plt->free(sb);
  return 0;
}

MyfsDiskInode *myfs_iget(MyfsSuperBlock *sb, uint32_t ino) {
  if (!bitmap_is_set(sb->inode_bitmap, ino))
    return NULL;

  MyfsDiskInode *inode = myfs_cache_find(sb->inode_hash_table, ino);
  if (inode) {
    inode->ref_count++;
    return inode;
  }

  inode = myfs_disk_inode_read(sb, ino);
  if (inode) {
    inode->ref_count = 1;
    inode->dirty = 1;
    myfs_inode_cache_add(sb, sb->inode_hash_table, inode);
  }
  return inode;
}

void myfs_iput(MyfsSuperBlock *sb, MyfsDiskInode *inode) {
  if (!inode)
    return;
  inode->ref_count--;
  if (inode->ref_count <= 0) {
    if (inode->dirty)
      myfs_disk_inode_write(sb, inode->i_ino);
    myfs_inode_cache_remove(sb, sb->inode_hash_table, inode->i_ino);
  }
}

static int myfs_blocks_alloc(MyfsSuperBlock *sb, uint32_t *block_array,
                             int count) {
  if (!block_array)
    return -1;
  for (int i = 0; i < count; i++) {
    int n = find_first_empty_bit(sb->block_bitmap,
                                 (sb->on_disk.total_blocks + 7) / 8,
                                 sb->on_disk.data_blocks_start);
    if (n == -1) {
      for (int j = 0; j < i; j++)
        bitmap_clear(sb->block_bitmap, block_array[j]);
      return -1;
    }
    block_array[i] = n;
    bitmap_set(sb->block_bitmap, n);
  }
  return 0;
}

static void myfs_blocks_free(MyfsSuperBlock *sb, uint32_t *block_list,
                             int count) {
  if (!block_list)
    return;
  for (int i = 0; i < count; i++)
    bitmap_clear(sb->block_bitmap, block_list[i]);
}

int myfs_inode_alloc(MyfsSuperBlock *sb, MyfsDiskInode **inode, int mode) {
  int inode_num =
      find_first_empty_bit(sb->inode_bitmap, (sb->on_disk.total_inodes + 7) / 8,
                           MYFS_ROOT_INODE_NUM);
  if (inode_num == -1)
    return -1;

  bitmap_set(sb->inode_bitmap, inode_num);

  /* Flush just the affected bitmap block */
  uint32_t bitmap_block_idx = inode_num / (sb->block_bytes * 8);
  size_t bitmap_size_bytes = (sb->on_disk.total_inodes + 7) / 8;
  int bytes_to_write =
      MIN((int)(bitmap_size_bytes - bitmap_block_idx * sb->block_bytes),
          (int)sb->block_bytes);
  disk_write_bitmap(sb, sb->on_disk.inode_bitmap_start + bitmap_block_idx,
                    sb->inode_bitmap + bitmap_block_idx * sb->block_bytes,
                    bytes_to_write);

  MyfsDiskInode *new_inode = sb->plt->alloc(sizeof(*new_inode));
  if (!new_inode) {
    bitmap_clear(sb->inode_bitmap, inode_num);
    return -1;
  }
  memset(new_inode, 0, sizeof(*new_inode));

  new_inode->i_ino = inode_num;
  new_inode->mode = mode;

  if (myfs_blocks_alloc(sb, new_inode->direct_blocks,
                        sb->on_disk.file_initial_blocks) < 0) {
    bitmap_clear(sb->inode_bitmap, inode_num);
    sb->plt->free(new_inode);
    return -1;
  }

  if (S_ISREG(mode))
    new_inode->block_count = sb->on_disk.file_initial_blocks;
  else if (S_ISDIR(mode))
    new_inode->block_count = 1;
  else if (S_ISLNK(mode))
    new_inode->block_count = 0;
  else
    new_inode->block_count = 1;

  new_inode->ref_count = 1;
  new_inode->links_count = 1;
  new_inode->permissions = S_ISDIR(mode) ? 0755 : 0644;
  new_inode->dirty = 1;

  uint64_t now = 0; /* replace with your kernel time source */
  new_inode->atime = now;
  new_inode->mtime = now;
  new_inode->ctime = now;

  *inode = new_inode;
  myfs_inode_cache_add(sb, sb->inode_hash_table, new_inode);
  myfs_disk_inode_write(sb, new_inode->i_ino);
  return 0;
}

static void myfs_inode_free(MyfsSuperBlock *sb, MyfsDiskInode *inode) {
  if (!inode)
    return;
  bitmap_clear(sb->inode_bitmap, inode->i_ino);
  myfs_blocks_free(sb, inode->direct_blocks, inode->block_count);
  myfs_inode_cache_remove(sb, sb->inode_hash_table, inode->i_ino);
}

uint32_t myfs_node_read(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                        uint32_t offset, void *buf, size_t count) {
  if (!inode || offset >= inode->size)
    return 0;
  if (offset + count > inode->size)
    count = inode->size - offset;

  uint32_t bytes_read = 0;
  uint8_t *out = (uint8_t *)buf;
  uint8_t block_buf[sb->block_bytes];

  while (bytes_read < count) {
    uint32_t block_idx = offset / sb->block_bytes;
    uint32_t block_offset = offset % sb->block_bytes;
    uint32_t to_read = MIN(sb->block_bytes - block_offset, count - bytes_read);

    if (block_idx >= inode->block_count)
      break;

    sb->plt->read_block(sb->dev, inode->direct_blocks[block_idx], block_buf);
    memcpy(out + bytes_read, block_buf + block_offset, to_read);

    offset += to_read;
    bytes_read += to_read;
  }

  inode->atime = 0;
  inode->dirty = 1;
  return bytes_read;
}

uint32_t myfs_node_write(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                         uint32_t offset, const void *buf, size_t count) {
  if (!inode || !buf || count == 0)
    return 0;

  int block_bytes = sb->block_bytes;
  int target_blocks = (offset + count + block_bytes - 1) / block_bytes;

  if (target_blocks > MYFS_DIRECT_BLOCKS_MAX)
    return -1;

  if (target_blocks > inode->block_count) {
    int additional = target_blocks - inode->block_count;
    if (myfs_blocks_alloc(sb, &inode->direct_blocks[inode->block_count],
                          additional) < 0)
      return -1;
    inode->block_count += additional;
  }

  uint32_t bytes_written = 0;
  const uint8_t *in = (const uint8_t *)buf;
  uint8_t block_buf[block_bytes];

  while (bytes_written < count) {
    uint32_t block_idx = offset / block_bytes;
    uint32_t block_offset = offset % block_bytes;
    uint32_t to_write = MIN(block_bytes - block_offset, count - bytes_written);

    if (to_write < (uint32_t)block_bytes)
      sb->plt->write_block(sb->dev, inode->direct_blocks[block_idx], block_buf);

    memcpy(block_buf + block_offset, in + bytes_written, to_write);
    sb->plt->write_block(sb->dev, inode->direct_blocks[block_idx], block_buf);

    offset += to_write;
    bytes_written += to_write;
  }

  if (offset > inode->size)
    inode->size = offset;

  inode->mtime = 0; /* replace with kernel time */
  inode->dirty = 1;
  myfs_disk_inode_write(sb, inode->i_ino);
  return bytes_written;
}

int myfs_inode_truncate(MyfsSuperBlock *sb, MyfsDiskInode *inode,
                        uint32_t new_size) {
  if (!inode)
    return -1;

  int block_bytes = sb->block_bytes;
  if (new_size < inode->size) {
    int new_blocks = (new_size + block_bytes - 1) / block_bytes;
    if (new_blocks < inode->block_count) {
      for (int i = new_blocks; i < inode->block_count; i++)
        bitmap_clear(sb->block_bitmap, inode->direct_blocks[i]);
      inode->block_count = new_blocks;
    }
  }

  inode->size = new_size;
  inode->mtime = 0; /* replace with kernel time */
  inode->dirty = 1;
  return 0;
}

int myfs_entry_idx_find(MyfsDiskDirEntry *entries, int entries_count,
                        const char *name) {
  for (int i = 0; i < entries_count; i++)
    if (strcmp(entries[i].file_name, name) == 0)
      return i;
  return -1;
}

int myfs_lookup(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode, const char *name,
                uint32_t *found_ino) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode))
    return -1;
  if (dir_inode->size == 0)
    return -1;

  int entries_count = dir_inode->size / sizeof(MyfsDiskDirEntry);
  MyfsDiskDirEntry *entries = sb->plt->alloc(dir_inode->size);
  if (!entries)
    return -1;

  if (myfs_node_read(sb, dir_inode, 0, entries, dir_inode->size) < 0) {
    sb->plt->free(entries);
    return -1;
  }

  int idx = myfs_entry_idx_find(entries, entries_count, name);
  if (idx == -1) {
    sb->plt->free(entries);
    return -1;
  }

  *found_ino = entries[idx].inode_num;
  sb->plt->free(entries);
  return 0;
}

int myfs_dir_add_entry(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode,
                       const char *name, uint32_t inode_num) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode))
    return -1;

  uint32_t existing;
  if (myfs_lookup(sb, dir_inode, name, &existing) == 0)
    return -1;

  MyfsDiskDirEntry e;
  e.inode_num = inode_num;
  strncpy(e.file_name, name, sizeof(e.file_name) - 1);
  e.file_name[sizeof(e.file_name) - 1] = '\0';

  return myfs_node_write(sb, dir_inode, dir_inode->size, &e, sizeof(e));
}

int myfs_dir_rm_entry(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode,
                      const char *name) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode))
    return -1;

  int entries_count = dir_inode->size / sizeof(MyfsDiskDirEntry);
  MyfsDiskDirEntry *entries = sb->plt->alloc(dir_inode->size);
  if (!entries)
    return -1;

  if (myfs_node_read(sb, dir_inode, 0, entries, dir_inode->size) < 0) {
    sb->plt->free(entries);
    return -1;
  }

  int idx = myfs_entry_idx_find(entries, entries_count, name);
  if (idx == -1) {
    sb->plt->free(entries);
    return -1;
  }

  for (int i = idx; i < entries_count - 1; i++)
    entries[i] = entries[i + 1];

  int new_size = (entries_count - 1) * sizeof(MyfsDiskDirEntry);
  myfs_node_write(sb, dir_inode, 0, entries, new_size);
  myfs_inode_truncate(sb, dir_inode, new_size);

  sb->plt->free(entries);
  return 0;
}

int myfs_readdir(MyfsSuperBlock *sb, MyfsDiskInode *dir_inode, uint32_t offset,
                 MyfsDiskDirEntry *entry) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode) || !entry)
    return -1;
  if (offset >= dir_inode->size)
    return 0;
  return myfs_node_read(sb, dir_inode, offset, entry, sizeof(MyfsDiskDirEntry));
}

/* -----------------------------------------------------------------------
 * rename / create / unlink
 * ---------------------------------------------------------------------- */

int myfs_rename(MyfsSuperBlock *sb, MyfsDiskInode *old_parent,
                const char *old_name, MyfsDiskInode *new_parent,
                const char *new_name) {
  uint32_t found_ino = 0;
  if (myfs_lookup(sb, old_parent, old_name, &found_ino) == -1)
    return -1;
  if (myfs_dir_rm_entry(sb, old_parent, old_name) == -1)
    return -1;

  myfs_unlink(sb, new_parent, new_name); /* ok if it doesn't exist */

  return myfs_dir_add_entry(sb, new_parent, new_name, found_ino);
}

int myfs_create_file(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                     const char *name, uint32_t *new_ino) {
  uint32_t existing;
  if (myfs_lookup(sb, parent_dir, name, &existing) == 0)
    return -1;

  MyfsDiskInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFREG) < 0)
    return -1;

  if (myfs_dir_add_entry(sb, parent_dir, name, new_inode->i_ino) < 0) {
    myfs_inode_free(sb, new_inode);
    return -1;
  }

  *new_ino = new_inode->i_ino;
  myfs_iput(sb, new_inode);
  return 0;
}

int myfs_create_dir(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                    const char *name, uint32_t *new_ino) {
  uint32_t existing;
  if (myfs_lookup(sb, parent_dir, name, &existing) == 0)
    return -1;

  MyfsDiskInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFDIR) < 0)
    return -1;

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

int myfs_unlink(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                const char *name) {
  uint32_t target_ino;
  if (myfs_lookup(sb, parent_dir, name, &target_ino) < 0)
    return -1;

  MyfsDiskInode *target = myfs_iget(sb, target_ino);
  if (target)
    myfs_inode_free(sb, target);

  return myfs_dir_rm_entry(sb, parent_dir, name);
}

uint32_t myfs_symlink_read(MyfsSuperBlock *sb, MyfsDiskInode *inode, char *buf,
                           size_t bufsize) {
  if (!S_ISLNK(inode->mode))
    return -1;
  return myfs_node_read(sb, inode, 0, buf, MIN(inode->size, bufsize));
}

int myfs_create_symlink(MyfsSuperBlock *sb, MyfsDiskInode *parent_dir,
                        const char *name, const char *target) {
  uint32_t existing;
  if (myfs_lookup(sb, parent_dir, name, &existing) == 0)
    return -1;

  MyfsDiskInode *new_inode;
  if (myfs_inode_alloc(sb, &new_inode, S_IFLNK) < 0)
    return -1;

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

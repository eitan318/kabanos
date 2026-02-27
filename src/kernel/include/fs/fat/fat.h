#pragma once

#include "drivers/block/blockdev.h"
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"
#include "ksys/stat.h"
#include "vfs.h"

/* -----------------------------------------------------------------------
 * FAT12 / FAT16 / FAT32 kernel driver
 * - Zero globals: all state lives in fat_fs_t, allocated per-mount
 * - Partition I/O abstracted via blkdev_t
 * - Thread-safe via per-fs spinlock (caller provides lock primitives)
 * ---------------------------------------------------------------------- */

#define FAT_SECTOR_SIZE 512
#define FAT_MAX_PATH 256
#define FAT_FILENAME_LEN 8
#define FAT_EXT_LEN 3
#define FAT_NAME_LEN 11 /* 8.3 packed */

/* FAT type */
typedef enum {
  FAT_TYPE_12 = 12,
  FAT_TYPE_16 = 16,
  FAT_TYPE_32 = 32,
} fat_type_t;

/* FAT directory entry attributes */
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN                                                           \
  (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

/* End-of-chain sentinels per FAT type */
#define FAT12_EOC 0x0FF8u
#define FAT16_EOC 0xFFF8u
#define FAT32_EOC 0x0FFFFFF8u
#define FAT32_MASK 0x0FFFFFFFu

/* -----------------------------------------------------------------------
 * On-disk structures (all little-endian, packed)
 * ---------------------------------------------------------------------- */

typedef struct __attribute__((packed)) {
  uint8_t jmp[3];
  uint8_t oem[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t root_entry_count; /* 0 for FAT32 */
  uint16_t total_sectors_16;
  uint8_t media;
  uint16_t sectors_per_fat_16; /* 0 for FAT32 */
  uint16_t sectors_per_track;
  uint16_t head_count;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;
} FAT_BPB; /* BIOS Parameter Block – common fields */

typedef struct __attribute__((packed)) {
  FAT_BPB bpb;
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_sig;
  uint32_t volume_id;
  uint8_t volume_label[11];
  uint8_t fs_type[8];
} FAT16_ExtBPB;

typedef struct __attribute__((packed)) {
  FAT_BPB bpb;
  uint32_t sectors_per_fat_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info_sector;
  uint16_t backup_boot_sector;
  uint8_t reserved[12];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_sig;
  uint32_t volume_id;
  uint8_t volume_label[11];
  uint8_t fs_type[8];
} FAT32_ExtBPB;

typedef struct __attribute__((packed)) {
  uint8_t name[11]; /* 8.3 packed, space-padded */
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenths;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t first_cluster_high; /* 0 for FAT12/16 */
  uint16_t write_time;
  uint16_t write_date;
  uint16_t first_cluster_low;
  uint32_t size;
} FAT_DirEntry; /* 32 bytes */

typedef struct __attribute__((packed)) {
  uint8_t order;
  uint16_t name1[5];
  uint8_t attributes; /* always FAT_ATTR_LFN */
  uint8_t type;
  uint8_t checksum;
  uint16_t name2[6];
  uint16_t first_cluster; /* always 0 */
  uint16_t name3[2];
} FAT_LFNEntry; /* 32 bytes */

/* -----------------------------------------------------------------------
 * Per-mount filesystem state  (heap-allocated by fat_mount())
 * ---------------------------------------------------------------------- */

typedef struct fat_fs fat_fs_t;
struct fat_fs {
  blkdev_t *dev;

  fat_type_t type;
  uint32_t bytes_per_sector;
  uint32_t sectors_per_cluster;
  uint32_t bytes_per_cluster;
  uint32_t fat_lba;     /* LBA of first FAT */
  uint32_t fat_sectors; /* sectors per FAT copy */
  uint8_t fat_count;
  uint32_t root_lba;      /* FAT12/16: LBA of root dir */
  uint32_t root_sectors;  /* FAT12/16: size in sectors */
  uint32_t root_cluster;  /* FAT32: root dir first cluster */
  uint32_t data_lba;      /* LBA of cluster 2 */
  uint32_t cluster_count; /* total data clusters */
  uint32_t total_sectors;

  /* In-memory FAT table (full copy, heap allocated) */
  uint8_t *fat_table;
  uint32_t fat_table_bytes;

  /* Sector cache (single-sector, direct-mapped) */
  uint8_t *sector_buf;
  uint32_t sector_buf_lba;
  bool sector_buf_dirty;

  /* Simple spinlock – set lock/unlock to your kernel primitives,
   * or leave NULL for single-threaded use. */
  void (*lock)(fat_fs_t *);
  void (*unlock)(fat_fs_t *);
};

/* -----------------------------------------------------------------------
 * Open file descriptor
 * ---------------------------------------------------------------------- */

typedef struct {
  fat_fs_t *fs;

  bool is_directory;
  bool is_root_dir;  /* FAT12/16 flat root */
  uint32_t size;     /* 0 for directories */
  uint32_t position; /* byte offset */

  uint32_t first_cluster;
  uint32_t current_cluster;
  uint32_t current_sector_in_cluster;

  /* Buffered sector for the current read position */
  uint8_t buf[FAT_SECTOR_SIZE];
} fat_file_t;

fat_fs_t *fat_mount(blkdev_t *dev);
void fat_unmount(fat_fs_t *fs);
fat_file_t *fat_open(fat_fs_t *fs, const char *path);
ssize_t fat_read(fat_file_t *file, void *buf, size_t size);
int fat_fstat(fat_file_t *file, fstat_t *fstat);
off_t fat_seek(fat_file_t *file, off_t offset);
void fat_close(fat_file_t *file);
bool fat_read_dir(fat_file_t *dir, FAT_DirEntry *out);
int fat_stat(fat_fs_t *fs, const char *path, FAT_DirEntry *out);
fat_file_t *fat_open_by_cluster(fat_fs_t *fs, uint32_t cluster, bool is_dir,
                                uint32_t size);
bool dir_find(fat_file_t *dir, const char *name, FAT_DirEntry *out);

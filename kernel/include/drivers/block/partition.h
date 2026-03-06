#include "drivers/block/blockdev.h"
#include "klib/stdint.h"
#define MBR_PARTITIONS 4

enum MBRPartitionEntryFlag {
  BOOTABLE = 0x80,
  NON_BOOTABLE = 0x00,
};

enum MBRPartitionEntryType {
  FAT12 = 0x01,
  FAT16 = 0x04,       // optional
  FAT32_CHS = 0x0B,   // FAT32 with CHS addressing
  FAT32_LBA = 0x0C,   // FAT32 LBA variant
  EXTENDED_0F = 0x0F, // extended partition
  EXTENDED_05 = 0x05, // extended partition (CHS)
  EXTENDED_85 = 0x85, // extended partition (Linux)
};

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // 0x01=FAT12, 0x0B/F=FAT32, etc
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t total_sectors;
} mbr_partition_entry_t;

typedef struct {
  blkdev_t *parent;
  uint32_t start_lba;
  uint32_t sector_count;
  int part_index;
} partition_info_t;

void partition_probe(blkdev_t *physical_dev);

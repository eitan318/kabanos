#pragma once
#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

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
} MBRPartitionEntry;

typedef struct {
  int entries_count;
  MBRPartitionEntry *partition_entries;
} partition_table_t;



typedef struct {
  DiskParams *disk;
  uint32_t partitionOffset;
  uint32_t partitionSize;
} Partition;

bool mbr_partition_table_get(DiskParams *disk, partition_table_t *partition_table);
bool Partition_read_sectors(Partition *part, uint32_t lba, uint8_t sectors,
                            void *lowerDataOut);

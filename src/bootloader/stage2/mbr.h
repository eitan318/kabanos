#pragma once
#include "arch/i686/disk.h"
#include <stdbool.h>
#include <stdint.h>

#define MBR_PARTITIONS 4

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // 0x01=FAT12, 0x0B/F=FAT32, etc
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t total_sectors;
} MBRPartitionEntry;

// External API: read partition table
bool mbr_read(DiskParams *disk);
const MBRPartitionEntry *mbr_get_partitions(void);
int mbr_get_partition_count(void);

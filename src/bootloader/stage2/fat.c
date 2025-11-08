#include "fat.h"
#include "stdio.h"
#include <stdint.h>

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // e.g., 0x0B = FAT32 CHS, 0x0C = FAT32 LBA
  uint8_t chs_end[3];     // CHS address of last sector
  uint32_t lba_start;     // LBA of first sector of partition
  uint32_t total_sectors; // number of sectors in partition
} MBRPartitionEntry;

typedef struct __attribute__((packed)) {
  uint8_t boot_code[446];
  MBRPartitionEntry partitions[4];
  uint16_t boot_signature; // should be 0xAA55
} MBR;

union {
  MBR mbr;
  uint8_t bytes[SECTOR_SIZE];
} g_mbr;

bool read_boot_sector(DiskParams *disk) {
  return disk_read_sectors(disk, 0, 1, g_mbr.bytes);
}
void check(DiskParams *disk) {
  if (!read_boot_sector(disk)) {
    debugf("Failed to read MBR\n");
    return;
  }

  if (g_mbr.mbr.boot_signature != 0xAA55) {
    debugf("Invalid MBR signature: 0x%04X\n", g_mbr.mbr.boot_signature);
    return;
  }

  debugf("MBR Partition Table:\n");
  debugf("Idx | Boot | Type |   LBA Start  | Total Sectors\n");
  debugf("-----------------------------------------------\n");

  for (int i = 0; i < 4; i++) {
    MBRPartitionEntry *p = &g_mbr.mbr.partitions[i];

    debugf("%d |  0x%X | 0x%X | %u | %u\n", i, p->boot_flag, p->partition_type,
           p->lba_start, p->total_sectors);
  }
}

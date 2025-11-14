#include "mbr.h"
#include <string.h> // optional for memset

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t boot_code[446];
  MBRPartitionEntry partitions[MBR_PARTITIONS];
  uint16_t boot_signature; // should be 0xAA55
} MBR;

// internal, invisible to outside
static union {
  MBR mbr;
  uint8_t bytes[SECTOR_SIZE];
} g_mbr;

// Read MBR from disk into g_mbr
bool mbr_read(DiskParams *disk) {
  if (!disk_read_sectors(disk, 0, 1, g_mbr.bytes)) {
    return false;
  }
  if (g_mbr.mbr.boot_signature != 0xAA55) {
    return false;
  }
  return true;
}

// Getter: pointer to partition table
const MBRPartitionEntry *mbr_get_partitions(void) {
  return g_mbr.mbr.partitions;
}

// Getter: number of partitions (always 4 in MBR)
int mbr_get_partition_count(void) { return MBR_PARTITIONS; }

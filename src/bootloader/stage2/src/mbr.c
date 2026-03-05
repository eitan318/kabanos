#include "mbr.h"
#include "disk.h"
#include "s2lib/string.h" // optional for memset

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t boot_code[446];
  mbr_partition_entry_t partitions[MBR_PARTITIONS];
  uint16_t boot_signature; // should be 0xAA55
} mbr_t;

static union {
  mbr_t mbr;
  uint8_t bytes[SECTOR_SIZE];
} mbr_union;

bool mbr_partition_table_get(DiskParams *disk,
                             partition_table_t *partition_table) {
  if (!disk_read_sectors(disk, 0, 1, mbr_union.bytes)) {
    return false;
  }
  if (mbr_union.mbr.boot_signature != 0xAA55) {
    return false;
  }

  partition_table->entries_count = MBR_PARTITIONS;
  partition_table->partition_entries = mbr_union.mbr.partitions;

  return true;
}

bool Partition_read_sectors(Partition *part, uint32_t lba, uint8_t sectors,
                            void *lowerDataOut) {
  return disk_read_sectors(part->disk, lba + part->partitionOffset, sectors,
                           lowerDataOut);
}

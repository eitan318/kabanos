#include "partition.h"
#include "drivers/ata.h"
#include <string.h> // optional for memset

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t boot_code[446];
  MBRPartitionEntry partitions[MBR_PARTITIONS];
  uint16_t boot_signature; // should be 0xAA55
} MBR;

static union {
  MBR mbr;
  uint8_t bytes[SECTOR_SIZE];
} g_mbr;

bool kmbr_partition_table_get(partition_table_t *partition_table) {
  ata_read_sector(0, 1, g_mbr.bytes);
  if (g_mbr.mbr.boot_signature != 0xAA55) {
    return false;
  }

  partition_table->entries_count = MBR_PARTITIONS;
  partition_table->partition_entries = g_mbr.mbr.partitions;

  return true;
}

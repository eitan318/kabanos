#include "fat.h"
#include "stdio.h"
#include <stdint.h>

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t boot_jump_instruction[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t dir_entry_count;
  uint16_t total_sectors;
  uint8_t media_descriptor_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t large_sector_count;

  // extended boot record
  uint8_t drive_number;
  uint8_t _reserved;
  uint8_t signature;
  uint32_t volume_id;       // serial number, value doesn't matter
  uint8_t volume_label[11]; // 11 bytes, padded with spaces
  uint8_t system_id[8];

} FatBootSector;

union {
  FatBootSector boot_sector;
  uint8_t boot_sector_bytes[SECTOR_SIZE];
} g_boot_sector;

bool read_boot_sector(DiskParams *disk) {
  return disk_read_sectors(disk, 0, 1, g_boot_sector.boot_sector_bytes);
}

void check(DiskParams *disk) {
  if (!read_boot_sector(disk)) {
    debugf("Failed to read boot sector\n");
    return;
  }
}

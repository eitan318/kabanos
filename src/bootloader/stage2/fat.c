#include "fat.h"
#include "stdio.h"
#include <stdint.h>

#define SECTOR_SIZE 512

typedef struct __attribute__((packed)) {
  uint8_t Name[11];
  uint8_t Attributes;
  uint8_t _Reserved;
  uint8_t CreatedTimeTenths;
  uint16_t CreatedTime;
  uint16_t CreatedDate;
  uint16_t AccessedDate;
  uint16_t FirstClusterHigh;
  uint16_t ModifiedTime;
  uint16_t ModifiedDate;
  uint16_t FirstClusterLow;
  uint32_t Size;
} FAT_DirectoryEntry;

typedef struct {
  int Handle;
  bool IsDirectory;
  uint32_t Position;
  uint32_t Size;
} FAT_File;

enum FAT_Attributes {
  FAT_ATTRIBUTE_READ_ONLY = 0x01,
  FAT_ATTRIBUTE_HIDDEN = 0x02,
  FAT_ATTRIBUTE_SYSTEM = 0x04,
  FAT_ATTRIBUTE_VOLUME_ID = 0x08,
  FAT_ATTRIBUTE_DIRECTORY = 0x10,
  FAT_ATTRIBUTE_ARCHIVE = 0x20,
  FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN |
                      FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

// FAT12 BPB structure (packed)
typedef struct __attribute__((packed)) {
  uint8_t jump[3];           // Jump instruction
  char oem[8];               // OEM name
  uint16_t bytes_per_sector; // Bytes per sector
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;         // Number of FATs
  uint16_t root_entries;     // Root directory entries
  uint16_t total_sectors_16; // Total sectors (if < 65536)
  uint8_t media_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32; // Total sectors (if >= 65536)
  // FAT12/16 extended
  uint8_t drive_number;
  uint8_t reserved;
  uint8_t signature; // 0x28 or 0x29
  uint32_t volume_id;
  char volume_label[11];
  char fs_type[8];
} fat12_bpb_t;

typedef struct {
  union {
    fat12_bpb_t BootSector;
    uint8_t BootSectorBytes[SECTOR_SIZE];
  } BS;

} FAT_Data;

static FAT_Data g_ActualData;
static uint32_t g_DataSectionLba;

bool fat_read_fat_boot_sector(DiskParams *disk, uint32_t fat_boot_sector_lba) {
  return disk_read_sectors(disk, fat_boot_sector_lba, 1,
                           g_ActualData.BS.BootSectorBytes);
}

void check(DiskParams *disk, uint32_t fat_boot_sector_lba) {
  if (!fat_read_fat_boot_sector(disk, fat_boot_sector_lba)) {
    debugf("Failed to read FAT boot sector at LBA %u\n", fat_boot_sector_lba);
    return;
  }

  fat12_bpb_t *bs = &g_ActualData.BS.BootSector;
  // The print in here coses problems
  // debugf("FAT Boot Sector:\n");
  // debugf("OEM Identifier: %.8s\n", bs->oem);
  // debugf("Bytes per Sector: %u\n", bs->bytes_per_sector);
  // debugf("Sectors per Cluster: %u\n", bs->sectors_per_cluster);
  // debugf("Reserved Sectors: %u\n", bs->reserved_sectors);
  // debugf("Number of FATs: %u\n", bs->fat_count);
  // debugf("Root Dir Entries: %u\n", bs->root_entries);
  // debugf("Total Sectors (small): %u\n", bs->total_sectors_16);
  // debugf("Media Descriptor Type: 0x%02X\n", bs->media_type);
  // debugf("Sectors per FAT: %u\n", bs->sectors_per_fat);
  // debugf("Sectors per Track: %u\n", bs->sectors_per_track);
  // debugf("Heads: %u\n", bs->heads);
  // debugf("Hidden Sectors: %u\n", bs->hidden_sectors);
  // debugf("Total Sectors (large): %u\n", bs->total_sectors_32);
  // debugf("Drive Number: 0x%02X\n", bs->drive_number);
  // debugf("Signature: 0x%02X\n", bs->signature);
  // debugf("Volume ID: 0x%08X\n", bs->volume_id);
  // debugf("Volume Label: %.11s\n", bs->volume_label);
  // debugf("System ID: %.8s\n", bs->fs_type);
}

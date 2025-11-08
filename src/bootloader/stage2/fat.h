#pragma once

#include "disk.h"
#include <stdint.h>

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

// FAT directory entry (32 bytes)
typedef struct __attribute__((packed)) {
  char name[11]; // 8.3 filename
  uint8_t attributes;
  uint8_t reserved;
  uint8_t create_time_fine;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t access_date;
  uint16_t cluster_high; // Always 0 for FAT12/16
  uint16_t modify_time;
  uint16_t modify_date;
  uint16_t cluster_low; // First cluster
  uint32_t file_size;
} fat12_dirent_t;

// FAT file attributes
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN 0x0F // Long filename

// Function prototypes
void fat_init(void);
int fat_load_file(const char *filename, void *buffer);

void check(DiskParams *disk);

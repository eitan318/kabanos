#pragma once
// kernel fat

#include <stdbool.h>
#include <stdint.h>

// Function prototypes
typedef struct {
  int handle;
  bool is_directory;
  uint32_t position;
  uint32_t size;
} FAT_File;

// fat.h - add these:

typedef struct __attribute__((packed)) {
  uint8_t name[11];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenths;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t first_cluster_high;
  uint16_t last_write_time;
  uint16_t last_write_date;
  uint16_t first_cluster_low;
  uint32_t size;
} FAT_DirectoryEntry;

typedef enum {
  FAT_ATTRIBUTE_READ_ONLY = 0x01,
  FAT_ATTRIBUTE_HIDDEN = 0x02,
  FAT_ATTRIBUTE_SYSTEM = 0x04,
  FAT_ATTRIBUTE_VOLUME_ID = 0x08,
  FAT_ATTRIBUTE_DIRECTORY = 0x10,
  FAT_ATTRIBUTE_ARCHIVE = 0x20,
  FAT_ATTRIBUTE_LFN = 0x0F,
} FAT_Attributes;

// Already public:
bool fat_initialize(uint32_t partition_lba);
FAT_File *fat_open(const char *path);
uint32_t fat_read(FAT_File *file, uint32_t byte_count, void *out);
bool fat_read_entry(FAT_File *file, FAT_DirectoryEntry *entry);
void fat_close(FAT_File *file);

// Expose these for fat_vfs.c:
bool fat_find_file(FAT_File *dir, const char *name, FAT_DirectoryEntry *out);
FAT_File *fat_open_entry(FAT_DirectoryEntry *entry);
void fat_shutdown(void);

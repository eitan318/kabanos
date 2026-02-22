#pragma once

#include "mbr.h"
#include <stdint.h>

// Function prototypes
typedef struct {
  int handle;
  bool is_directory;
  uint32_t position;
  uint32_t size;
} FAT_File;

typedef struct __attribute__((packed)) {
  uint8_t name[11];
  uint8_t attributes;
  uint8_t reserved0;
  uint8_t created_time_tenths;
  uint16_t created_time;
  uint16_t created_date;
  uint16_t accessed_date;
  uint16_t first_cluster_high;
  uint16_t modified_time;
  uint16_t modified_date;
  uint16_t first_cluster_low;
  uint32_t size;
} FAT_DirectoryEntry;

int fat_read_file(const char *path, void *buffer);
FAT_File *fat_open(Partition *disk, const char *path);
bool fat_find_file(Partition *disk, FAT_File *file, const char *name,
                   FAT_DirectoryEntry *out);

bool fat_initialize(Partition *disk);

uint32_t fat_read(Partition *disk, FAT_File *file, uint32_t byteCount,
                  void *dataOut);
bool fat_read_entry(Partition *disk, FAT_File *file,
                    FAT_DirectoryEntry *dirEntry);
void fat_close(FAT_File *file);

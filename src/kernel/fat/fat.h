#pragma once

#include <stdbool.h>
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

// Initialize FAT filesystem with partition offset
bool fat_initialize(uint32_t partition_lba);

// Open a file
FAT_File *fat_open(const char *path);

// Read from file
uint32_t fat_read(FAT_File *file, uint32_t byte_count, void *out);

// Read directory entry
bool fat_read_entry(FAT_File *file, FAT_DirectoryEntry *entry);

// Close file
void fat_close(FAT_File *file);

// Helper: read entire file into buffer
int fat_read_file(const char *path, void **buffer, uint32_t *size);

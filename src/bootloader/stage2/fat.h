#pragma once

#include "disk.h"
#include <stdint.h>

// Function prototypes

int fat_read_file(const char *path, void *buffer);
bool fat_initialize(DiskParams *disk, uint32_t partition_lba);
int check(DiskParams *disk, uint32_t fat_boot_sector_lba);

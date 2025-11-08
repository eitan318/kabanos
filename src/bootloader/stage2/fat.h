#pragma once

#include "disk.h"
#include <stdint.h>

// Function prototypes
void fat_init(void);
int fat_load_file(const char *filename, void *buffer);

bool fat_read_fat_boot_sector(DiskParams *disk, uint32_t fat_boot_sector_lba);

void check(DiskParams *disk, uint32_t fat_boot_sector_lba);

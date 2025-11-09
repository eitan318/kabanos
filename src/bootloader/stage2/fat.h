#pragma once

#include "disk.h"
#include <stdint.h>

// Function prototypes

int FAT_ReadFile(const char *path, void *buffer);
bool FAT_Initialize(DiskParams *disk, uint32_t partition_lba);
int check(DiskParams *disk, uint32_t fat_boot_sector_lba);

#pragma once
#include "boot/bootparams.h" // <-- for DiskParams
#include <stdbool.h>
#include <stdint.h>

bool disk_read_sectors(const DiskParams *disk_params, uint32_t lba,
                       uint16_t total_count, void *dest);

bool disk_init(uint8_t drive_number, DiskParams *diskParams);

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct DiskParams {
    uint8_t hdds_count;
    uint8_t drive_id;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
    bool lba_support;
} DiskParams;

bool disk_read_sectors(const DiskParams* disk_params, uint32_t lba,
                       uint16_t total_count, void* dest);

bool disk_init(uint8_t drive_number, DiskParams* diskParams);

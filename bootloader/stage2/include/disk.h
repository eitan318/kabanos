/**
 * @file disk.h
 * @brief BIOS disk access (INT 13h) with CHS/LBA support.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct disk_params {
  uint8_t hdds_count;
  uint8_t drive_id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;
  bool lba_support;
} disk_params_t;

bool disk_read_sectors(const disk_params_t *disk_params, uint32_t lba,
                       uint16_t total_count, void *dest);

bool disk_init(uint8_t drive_number, disk_params_t *diskParams);

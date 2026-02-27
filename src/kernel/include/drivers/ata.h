#pragma once
#include "klib/stdint.h"

#define SECTOR_SIZE 512

int ata_write_sector(uint32_t lba, int count, const uint8_t *buffer);

void ata_read_sector(uint32_t lba, int count, uint8_t *buffer);
void ata_init(void);

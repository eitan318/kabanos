#pragma once
#include "boot/bootparams.h"
#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

#define MBR_PARTITIONS 4

typedef struct {
  DiskParams *disk;
  uint32_t partitionOffset;
  uint32_t partitionSize;
} Partition;

bool mbr_partition_table_get(DiskParams *disk, PartitionTable *partition_table);
bool Partition_read_sectors(Partition *part, uint32_t lba, uint8_t sectors,
                            void *lowerDataOut);

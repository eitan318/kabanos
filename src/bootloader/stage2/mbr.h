#pragma once
#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

#define MBR_PARTITIONS 4

// External API: read partition table
bool mbr_read(DiskParams *disk);
const MBRPartitionEntry *mbr_get_partitions(void);
int mbr_get_partition_count(void);

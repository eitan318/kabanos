#pragma once

#include "disk.h"
#include "mbr.h"
#include <stdint.h>

// Function prototypes

int fat_read_file(const char *path, void *buffer);
bool fat_initialize(Partition *disk);

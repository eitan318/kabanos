#pragma once

#include "device.h"
#include "klib/stdint.h"

#define MAX_BLOCK_DEVICES 50

#define SECTOR_SIZE 512

typedef struct blkdev blkdev_t;
struct blkdev {
  char name[32];
  device_t *generic; // Link to your generic device table entry

  // Function pointers for disk I/O
  int (*read_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count, void *buf);
  int (*write_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count,
                       const void *buf);

  void *priv;
};

int blkdev_register(blkdev_t *dev);
blkdev_t *blkdev_get(const char *name);

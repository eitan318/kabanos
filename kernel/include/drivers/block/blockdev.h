/**
 * @file blockdev.h
 * @brief Block device registry and sector-level I/O interface.
 */
#pragma once
#include "device.h"
#include "klib/stdint.h"

#define MAX_BLOCK_DEVICES 50

#define SECTOR_SIZE 512

typedef struct blkdev blkdev_t;

/** @brief A registered block device (disk or partition). */
struct blkdev {
  char name[32];     /**< Unique name, e.g. "ata0" or "ata0p1". */
  device_t *generic; /**< Corresponding generic device table entry. */

  /** @brief Reads @p count sectors starting at @p lba into @p buf. */
  int (*read_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count, void *buf);
  /** @brief Writes @p count sectors starting at @p lba from @p buf. */
  int (*write_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count,
                       const void *buf);

  uint32_t sectors; /**< Total capacity in sectors. */
  void *priv;       /**< Driver-private state. */
};

/**
 * @brief Adds @p dev to the global block device table.
 * @return 0 on success, negative value if the table is full.
 */
int blkdev_register(blkdev_t *dev);

/** @brief Looks up a block device by name; NULL if not found. */
blkdev_t *blkdev_get(const char *name);

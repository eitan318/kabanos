/**
 * @file blockdev.c
 * @brief Block device registry.
 */
#include "drivers/block/blockdev.h"
#include "klib/errno.h"
#include "klib/string.h"

static blkdev_t *g_blkdevs[MAX_BLOCK_DEVICES];

int blkdev_register(blkdev_t *dev) {
  for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
    if (g_blkdevs[i] == NULL) {
      g_blkdevs[i] = dev;
      return 0;
    }
  }
  return -ENOMEM;
}

blkdev_t *blkdev_get(const char *name) {
  for (int i = 0; i < MAX_BLOCK_DEVICES; i++) {
    if (g_blkdevs[i] && strcmp(g_blkdevs[i]->name, name) == 0) {
      return g_blkdevs[i];
    }
  }
  return NULL;
}

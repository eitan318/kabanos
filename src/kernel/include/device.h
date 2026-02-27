#pragma once
#include "sched/wait.h"

typedef struct {
  int device_id;
  bool data_ready;
  char buffer[256];

  // Every device carries its own "waiting room"
  wait_queue_t wait_queue;
} device_t;

typedef enum {
  DEVICE_HANDLE_KEYBOARD = 1,
  DEVICE_HANDLE_ATA = 2,
} device_handle_t;

device_t *get_device_by_handle(int handle);
device_t *device_init(int id);

typedef struct blkdev blkdev_t;
struct blkdev {
  /* Read/write `count` 512-byte sectors starting at `lba` into `buf`.
   * Returns 0 on success, negative errno on failure. */
  int (*read_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count, void *buf);
  int (*write_sectors)(blkdev_t *dev, uint32_t lba, uint32_t count,
                       const void *buf);
  void *priv; /* driver-private data */
};

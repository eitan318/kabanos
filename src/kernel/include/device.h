#pragma once
#include "klib/string.h"
#include "sched/wait.h"

typedef struct device device_t;
typedef struct device {
  int device_id;
  bool data_ready;
  wait_queue_t wait_queue;

  // char buffer[256];

  struct device_ops *ops;
  void *priv;
} device_t;

struct device_ops {
  ssize_t (*read)(device_t *dev, void *buf, size_t size);
  ssize_t (*write)(device_t *dev, const void *buf, size_t size);
};

typedef enum {
  DEVICE_HANDLE_KEYBOARD = 1,
  DEVICE_HANDLE_ATA = 2,
  DEVICE_HANDLE_CONSOLE = 3,
} device_handle_t;

device_t *get_device_by_handle(int handle);
device_t *device_init(int id);

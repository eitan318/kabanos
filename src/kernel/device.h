#pragma once
#include "sched/spinlock.h"
#include "sched/thread.h"
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
void kernel_init_hardware();

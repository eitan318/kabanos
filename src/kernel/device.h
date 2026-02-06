#pragma once
#include "sched/spinlock.h"
#include "sched/thread.h"

typedef struct wait_queue {
  thread_t *head;
  thread_t *tail;
  spinlock_t lock;
} wait_queue_t;

typedef struct {
  int device_id;
  bool data_ready;
  char buffer[256];

  // Every device carries its own "waiting room"
  wait_queue_t wait_queue;
} device_t;

device_t *get_device_by_handle(int handle);
device_t *device_init(int id);

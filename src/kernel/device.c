#include "device.h"
#include "kmalloc.h"
#include "sched/spinlock.h"
#include <stdbool.h>
#include <stddef.h>

device_t *device_add(int id) {
  device_t *dev = kmalloc(sizeof(device_t));
  dev->device_id = id;
  dev->data_ready = false;

  // Initialize the spinlock and pointers for this specific queue
  dev->wait_queue.head = NULL;
  dev->wait_queue.tail = NULL;
  dev->wait_queue.lock = SPINLOCK_RELEASED;
  return dev;
}

#define MAX_DEVICES 32

// A global registry of all detected hardware
static device_t *g_device_table[MAX_DEVICES];

device_t *get_device_by_handle(int handle) {
  if (handle < 0 || handle >= MAX_DEVICES) {
    return NULL;
  }
  return g_device_table[handle];
}

void kernel_init_hardware() {
  device_t *kbd = kmalloc(sizeof(device_t));
  device_init(kbd, 0);
}

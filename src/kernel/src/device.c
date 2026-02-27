#include "device.h"
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "mm/kmalloc.h"
#include "sched/dispatcher.h"
#include "sched/spinlock.h"
#include "sched/thread.h"

#define MAX_DEVICES 32

// A global registry of all detected hardware
static device_t *g_device_table[MAX_DEVICES];

device_t *device_init(int id) {
  device_t *dev = kmalloc(sizeof(device_t));
  if (!dev)
    return NULL; // Always check for kmalloc failure!

  dev->device_id = id;
  dev->data_ready = false;

  dev->wait_queue.head = NULL;
  dev->wait_queue.tail = NULL;

  dev->wait_queue.lock = (spinlock_t)SPINLOCK_RELEASED;

  g_device_table[id] = dev;

  return dev;
}

void device_destroy(device_t *dev) {
  if (!dev)
    return;

  // Safety check: ensure no threads are still sleeping here!
  if (dev->wait_queue.head != NULL) {
    // Handle error: You can't delete a device people are waiting on!
    return;
  }

  kfree(dev);
}

device_t *get_device_by_handle(int handle) {
  if (handle < 0 || handle >= MAX_DEVICES)
    return NULL;

  device_t *dev = g_device_table[handle];

  return dev;
}

int kernel_init_devices(module_t *module) {
  device_t *kbd = device_init(DEVICE_HANDLE_KEYBOARD);
  device_t *ata = device_init(DEVICE_HANDLE_ATA);
  return 0;
}

static const char *devices_deps[] = {"hal", NULL};

ITER_MODULE(devices) = {
    .name = "devices",
    .required = devices_deps,
    .init = &kernel_init_devices,
    .fini = NULL,
};

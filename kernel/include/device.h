/**
 * @file device.h
 * @brief Generic character-device abstraction.
 *
 * Devices are registered under fixed integer handles and expose their
 * functionality through a table of operations (@ref device_ops).
 */
#pragma once
#include "klib/string.h"
#include "ksys/types.h"
#include "sched/wait.h"

typedef struct device device_t;

/** @brief A registered device instance. */
typedef struct device {
  int device_id;     /**< Handle this device is registered under. */
  bool data_ready;   /**< Set by the driver when input is available. */
  wait_queue_t waitq; /**< Threads blocked waiting for @ref data_ready. */

  struct device_ops *ops; /**< Driver-provided operations table. */
  void *priv;             /**< Driver-private state. */
} device_t;

/** @brief Operations a driver implements for its device. */
struct device_ops {
  ssize_t (*read)(device_t *dev, void *buf, size_t size);
  ssize_t (*write)(device_t *dev, const void *buf, size_t size);
  int (*ioctl)(device_t *dev, uint32_t request, void *arg);
};

/** @brief Well-known device handles. */
typedef enum {
  DEVICE_HANDLE_KEYBOARD = 1,
  DEVICE_HANDLE_ATA = 2,
  DEVICE_HANDLE_CONSOLE = 3,
} device_handle_t;

/**
 * @brief Looks up a registered device by its handle.
 * @return The device, or NULL if the handle is unknown.
 */
device_t *get_device_by_handle(int handle);

/**
 * @brief Allocates and registers a new device under @p id.
 * @return The new device, or NULL on allocation failure.
 */
device_t *device_init(int id);

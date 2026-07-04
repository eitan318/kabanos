/**
 * @file keyboard_dev.c
 * @brief Keyboard character-device registration (stdin).
 */
#include "device.h"
#include "fs/vfs.h"
#include "fs/vfs_internal.h"
#include "klib/errno.h"
#include "ksys/stat.h"

static ssize_t kbd_dev_read(file_t *file, void *buf, size_t size) {
  int handle = file->vnode->device_handle;
  device_t *dev = get_device_by_handle(handle);
  if (dev && dev->ops && dev->ops->read)
    return dev->ops->read(dev, buf, size);
  return -ENODEV;
}

static file_ops_t kbd_fops = {.read = kbd_dev_read};
static super_block_t kbd_sb = {.f_ops = &kbd_fops};
vnode_t kbd_vnode_static = {
    .super_block = &kbd_sb,
    .mode = S_IFCHR,
    .refcount = 1,
    .device_handle = DEVICE_HANDLE_KEYBOARD,
};

/**
 * @file sys_ioctl.c
 * @brief ioctl syscall: routes requests to the device behind a
 *        character-device fd.
 */
#include "sys_ioctl.h"
#include "device.h"
#include "drivers/console/tty.h"
#include "fs/fd.h"
#include "klib/errno.h"
#include "ksys/stat.h"

int sys_ioctl(int fd, uint32_t request, void *arg) {
  if (fd < 0 || fd >= MAX_FD)
    return -EBADF;

  file_t *file = g_fd_table[fd];
  if (!file || !file->vnode)
    return -EBADF;

  if (!S_ISCHR(file->vnode->mode)) {
    return -ENOTTY;
  }

  device_t *dev = get_device_by_handle(file->vnode->device_handle);
  if (!dev || !dev->ops->ioctl)
    return -EINVAL;

  return dev->ops->ioctl(dev, request, arg);
}

/**
 * @file serial_dev.c
 * @brief Debug serial output device (fd 3).
 */
#include "fs/vfs.h"
#include "fs/vfs_internal.h"
#include "hal.h"
#include "ksys/stat.h"

static ssize_t serial_dev_write(file_t *file, const void *buf, size_t size) {
  const char *data = buf;
  for (size_t i = 0; i < size; i++)
    hal_serial_putc(data[i]);
  return size;
}

static file_ops_t dbg_fops = {.write = serial_dev_write};
static super_block_t dbg_sb = {.f_ops = &dbg_fops};
vnode_t dbg_vnode_static = {
    .super_block = &dbg_sb,
    .mode = S_IFCHR,
    .refcount = 1,
};

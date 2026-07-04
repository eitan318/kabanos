/**
 * @file console_dev.c
 * @brief Console character-device registration (vnode + device ops).
 */
#include "drivers/console/console.h"
#include "fs/vfs.h"
#include "fs/vfs_internal.h"
#include "ksys/stat.h"

#include <klib/stdio.h>
static ssize_t console_dev_write(file_t *file, const void *buf, size_t size) {
  const char *data = buf;
  for (size_t i = 0; i < size; i++) {
    con_putc(data[i]);
    kdebugc(data[i]);
  }
  return size;
}

static file_ops_t con_fops = {.write = console_dev_write};
static super_block_t con_sb = {.f_ops = &con_fops};
vnode_t con_vnode_static = {
    .super_block = &con_sb,
    .mode = S_IFCHR,
    .refcount = 1,
};

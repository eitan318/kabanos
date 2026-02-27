#include "fs/fd.h"
#include "device.h"
#include "drivers/console/vga_text.h"
#include "drivers/keyboard/keyboard.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "ksys/fcntl.h"
#include "ksys/stat.h"
#include "mm/kmalloc.h"

// An adapter for the VGA console
ssize_t dev_console_write(file_t *file, const void *buf, size_t size) {
  const char *data = (const char *)buf;
  for (size_t i = 0; i < size; i++) {
    vga_putc(data[i]);
  }
  return size; // Return how many bytes were "written"
}

// An adapter for the VGA console
ssize_t dev_debug_write(file_t *file, const void *buf, size_t size) {
  const char *data = (const char *)buf;
  for (size_t i = 0; i < size; i++) {
    hal_serial_putc(data[i]);
  }
  return size; // Return how many bytes were "written"
}

ssize_t vfs_device_read_bridge(file_t *file, void *buf, size_t size) {
  int handle = file->vnode->device_handle;

  device_t *dev = get_device_by_handle(handle);

  if (dev && dev->ops && dev->ops->read) {
    return dev->ops->read(dev, buf, size);
  }

  return -ENODEV;
}

file_t *g_fd_table[MAX_FD];

static file_ops_t kbd_fops = {.read = vfs_device_read_bridge};
static file_ops_t con_fops = {.write = dev_console_write};
static file_ops_t dbg_fops = {.write = dev_debug_write};

// We create a "dummy" superblock for hardware
static super_block_t dev_sb_kbd = {.f_ops = &kbd_fops};
static super_block_t dev_sb_con = {.f_ops = &con_fops};
static super_block_t dev_sb_dbg = {.f_ops = &dbg_fops};

// These will act as our "devfs_get_vnode" replacements for now
static vnode_t kbd_vnode_static;
static vnode_t con_vnode_static;
static vnode_t dbg_vnode_static;

void vfs_init_stdio(void) {
  kbd_vnode_static.super_block = &dev_sb_kbd;
  kbd_vnode_static.mode = S_IFCHR;
  kbd_vnode_static.refcount = 1;

  con_vnode_static.super_block = &dev_sb_con;
  con_vnode_static.mode = S_IFCHR;
  con_vnode_static.refcount = 1;

  dbg_vnode_static.super_block = &dev_sb_dbg;
  dbg_vnode_static.mode = S_IFCHR;
  dbg_vnode_static.refcount = 1;

  g_fd_table[0] = kmalloc(sizeof(file_t));
  g_fd_table[0]->vnode = &kbd_vnode_static;
  g_fd_table[0]->f_ops = kbd_vnode_static.super_block->f_ops;
  g_fd_table[0]->flags = O_RDONLY;

  g_fd_table[1] = kmalloc(sizeof(file_t));
  g_fd_table[1]->vnode = &con_vnode_static;
  g_fd_table[1]->f_ops = con_vnode_static.super_block->f_ops;
  g_fd_table[1]->flags = O_WRONLY;

  g_fd_table[2] = kmalloc(sizeof(file_t));
  memcpy(g_fd_table[2], g_fd_table[1], sizeof(file_t));

  g_fd_table[3] = kmalloc(sizeof(file_t));
  g_fd_table[3]->vnode = &dbg_vnode_static;
  g_fd_table[3]->f_ops = dbg_vnode_static.super_block->f_ops;
  g_fd_table[3]->flags = O_WRONLY;
}

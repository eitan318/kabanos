#include "vfs.h"
#include "e9.h"
#include "vga_text.h"

size_t pvfs_write(int fd, uint8_t *buf, size_t size) {
  switch (fd) {
  case VFS_FD_STDIN:
    return 0;
  case VFS_FD_STDOUT:
  case VFS_FD_STDERR:
    for (size_t i = 0; i < size; i++) {
      char c = buf[i];
      vga_putc(c);
    }
    return size;

  case VFS_FD_DEBUG:
    for (size_t i = 0; i < size; i++) {
      char c = buf[i];
      e9_putc(c);
    }
    return size;
  default:
    return -1;
  }
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs/fs_common.h"
#include "fs/myfs/myfs.h"
#define SECTOR_BYTES 512

typedef struct {
  FILE *fp;
  size_t offset_bytes;
  size_t size_bytes;
} HostDevice;

static int host_read_block(void *dev_handle, uint32_t lba, void *buf) {
  HostDevice *dev = (HostDevice *)dev_handle;
  size_t absolute_pos = dev->offset_bytes + ((size_t)lba * SECTOR_BYTES);

  if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  return (fread(buf, SECTOR_BYTES, 1, dev->fp) == 1) ? 0 : -1;
}

static int host_write_block(void *dev_handle, uint32_t lba, const void *buf) {
  HostDevice *dev = (HostDevice *)dev_handle;
  size_t absolute_pos = dev->offset_bytes + ((size_t)lba * SECTOR_BYTES);

  if (fseek(dev->fp, absolute_pos, SEEK_SET) != 0)
    return -1;
  if (fwrite(buf, SECTOR_BYTES, 1, dev->fp) != 1)
    return -1;
  fflush(dev->fp);
  return 0;
}

fs_platform_t *host_platform_create() {
  fs_platform_t *plt = malloc(sizeof(fs_platform_t));
  plt->alloc = malloc;
  plt->free = free;
  plt->read_block = host_read_block;
  plt->write_block = host_write_block;
  plt->log = (void *)printf;
  return plt;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <img> <offset_bytes> <size_bytes>\n", argv[0]);
    return 1;
  }

  // 1. Prepare the Device Handle
  HostDevice dev;
  dev.fp = fopen(argv[1], "r+b");
  if (!dev.fp) {
    perror("Failed to open image");
    return 1;
  }
  dev.offset_bytes = (size_t)strtoull(argv[2], NULL, 0);
  dev.size_bytes = (size_t)strtoull(argv[3], NULL, 0);

  // 2. Prepare the Platform Interface
  fs_platform_t *host_plt = host_platform_create();

  // 3. Call the Logic
  // Total blocks = size / block_size (assuming 512 for now)
  uint32_t total_blocks = dev.size_bytes / SECTOR_BYTES;

  if (myfs_format(&dev, host_plt) < 0) {
    fprintf(stderr, "myfs_format failed\n");
    fclose(dev.fp);
    return 1;
  }

  printf("Success: %s formatted (%u blocks)\n", argv[1], total_blocks);

  fclose(dev.fp);
  free(host_plt);
  return 0;
}

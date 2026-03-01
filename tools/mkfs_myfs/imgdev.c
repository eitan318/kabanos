#include "include/blockdev.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_BYTES 512

typedef struct {
  FILE *fp;
  size_t byte_offset; /* start of partition in file */
  size_t byte_size;   /* length of partition in file */
} ImgDevPriv;

static int img_read_sectors(blkdev_t *dev, uint32_t lba, uint32_t count,
                            void *buf) {
  ImgDevPriv *p = dev->priv;
  fseek(p->fp, p->byte_offset + (size_t)lba * SECTOR_BYTES, SEEK_SET);
  fread(buf, SECTOR_BYTES, count, p->fp);
  return 0;
}

static int img_write_sectors(blkdev_t *dev, uint32_t lba, uint32_t count,
                             const void *buf) {
  ImgDevPriv *p = dev->priv;
  fseek(p->fp, p->byte_offset + (size_t)lba * SECTOR_BYTES, SEEK_SET);
  fwrite(buf, SECTOR_BYTES, count, p->fp);
  fflush(p->fp);
  return 0;
}

blkdev_t *imgdev_open(const char *path, size_t byte_offset, size_t byte_size) {
  ImgDevPriv *p = malloc(sizeof(*p));
  p->fp = fopen(path, "r+b");
  if (!p->fp) {
    free(p);
    return NULL;
  }
  p->byte_offset = byte_offset;
  p->byte_size = byte_size;

  blkdev_t *dev = malloc(sizeof(*dev));
  dev->read_sectors = img_read_sectors;
  dev->write_sectors = img_write_sectors;
  dev->priv = p;
  return dev;
}

void imgdev_close(blkdev_t *dev) {
  ImgDevPriv *p = dev->priv;
  fclose(p->fp);
  free(p);
  free(dev);
}

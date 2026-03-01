#include "imgdev.h"
#include "myfs.h"
#include <stdio.h>
#include <stdlib.h>

/* Usage: mkfs.myfs <image.img> <byte_offset> <byte_size> */
int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "usage: %s <img> <offset_bytes> <size_bytes>\n", argv[0]);
    return 1;
  }

  const char *img = argv[1];
  size_t offset = (size_t)strtoull(argv[2], NULL, 0);
  size_t size = (size_t)strtoull(argv[3], NULL, 0);

  blkdev_t *dev = imgdev_open(img, offset, size);
  if (!dev) {
    perror("imgdev_open");
    return 1;
  }

  if (myfs_format(dev) < 0) {
    fprintf(stderr, "myfs_format failed\n");
    imgdev_close(dev);
    return 1;
  }

  printf("myfs formatted: %s @ offset %zu, size %zu bytes\n", img, offset,
         size);
  imgdev_close(dev);
  return 0;
}

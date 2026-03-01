#include "blockdev.h"
blkdev_t *imgdev_open(const char *path, size_t byte_offset, size_t byte_size);
void imgdev_close(blkdev_t *dev);

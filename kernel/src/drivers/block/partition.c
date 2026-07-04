/**
 * @file partition.c
 * @brief MBR parsing; exposes partitions as virtual block devices.
 */
#include "drivers/block/partition.h"
#include "drivers/block/blockdev.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "mm/kmalloc.h"

static int part_read(blkdev_t *dev, uint32_t lba, uint32_t count, void *buf) {
  partition_info_t *info = (partition_info_t *)dev->priv;

  if (lba + count > info->sector_count) {
    return -EINVAL;
  }

  return info->parent->read_sectors(info->parent, info->start_lba + lba, count,
                                    buf);
}

static int part_write(blkdev_t *dev, uint32_t lba, uint32_t count,
                      const void *buf) {
  partition_info_t *info = (partition_info_t *)dev->priv;

  if (lba + count > info->sector_count) {
    return -EINVAL;
  }

  return info->parent->write_sectors(info->parent, info->start_lba + lba, count,
                                     buf);
}

typedef struct __attribute__((packed)) {
  uint8_t boot_code[446];
  mbr_partition_entry_t partitions[MBR_PARTITIONS];
  uint16_t boot_signature; // should be 0xAA55
} mbr_t;

void partition_probe(blkdev_t *physical_dev) {
  union {
    mbr_t mbr;
    uint8_t bytes[SECTOR_SIZE];
  } mbr_union;

  if (physical_dev->read_sectors(physical_dev, 0, 1, mbr_union.bytes) < 0)
    return;

  mbr_t mbr = mbr_union.mbr;

  if (mbr.boot_signature != 0xaa55) {
    return;
  }

  for (int i = 0; i < 4; i++) {
    if (mbr.partitions[i].partition_type == 0)
      continue; // Skip empty slots

    // 1. Create the partition metadata
    partition_info_t *info = kmalloc(sizeof(partition_info_t));
    info->parent = physical_dev;
    info->start_lba = mbr.partitions[i].lba_start;
    info->sector_count = mbr.partitions[i].total_sectors;
    info->part_index = i + 1;

    // 2. Create the virtual block device
    blkdev_t *vdev = kmalloc(sizeof(blkdev_t));
    memset(vdev, 0, sizeof(blkdev_t));

    // Name it (e.g., "ata" + "p1" = "atap1")
    ksprintf(vdev->name, "%sp%d", physical_dev->name, i + 1);

    vdev->priv = info;
    vdev->read_sectors = part_read;
    vdev->write_sectors = part_write;
    vdev->sectors = info->sector_count;

    // 3. Register it so VFS can see it
    blkdev_register(vdev);

    kdebugf("Partition found: %s start=%u size=%u\n", vdev->name,
            info->start_lba, info->sector_count);
  }
}

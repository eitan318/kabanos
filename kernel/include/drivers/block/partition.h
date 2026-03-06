/**
 * @file partition.h
 * @brief MBR Partition Table parsing and management.
 * * Provides structures to decode the Master Boot Record (MBR) found on
 * block devices and abstractions to treat partitions as independent
 * logical block devices.
 */

#pragma once
#include "drivers/block/blockdev.h"
#include "klib/stdint.h"

/** @brief Number of primary partition entries in a standard MBR. */
#define MBR_PARTITIONS 4

/** @brief Boot indicator flags for MBR partitions. */
enum MBRPartitionEntryFlag {
  BOOTABLE = 0x80,     /**< Partition is active/bootable. */
  NON_BOOTABLE = 0x00, /**< Partition is inactive. */
};

/** @brief Common MBR Partition System IDs. */
enum MBRPartitionEntryType {
  FAT12 = 0x01,       /**< FAT12 file system. */
  FAT16 = 0x04,       /**< FAT16 (less than 32MB). */
  FAT32_CHS = 0x0B,   /**< FAT32 using CHS addressing. */
  FAT32_LBA = 0x0C,   /**< FAT32 using LBA addressing. */
  EXTENDED_0F = 0x0F, /**< Extended partition (LBA). */
  EXTENDED_05 = 0x05, /**< Extended partition (CHS). */
  EXTENDED_85 = 0x85, /**< Linux extended partition. */
};

/**
 * @struct mbr_partition_entry_t
 * @brief The 16-byte MBR partition record structure.
 * * This structure is byte-packed to match the physical layout on disk
 * within the boot sector (starting at offset 446).
 */
typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      /**< 0x80 for bootable, 0x00 otherwise. */
  uint8_t chs_start[3];   /**< Cylinder-Head-Sector start address (legacy). */
  uint8_t partition_type; /**< System ID / Partition type. */
  uint8_t chs_end[3];     /**< Cylinder-Head-Sector end address (legacy). */
  uint32_t lba_start;     /**< Logical Block Address of the first sector. */
  uint32_t total_sectors; /**< Number of sectors in the partition. */
} mbr_partition_entry_t;

/**
 * @struct partition_info_t
 * @brief Metadata for an identified logical partition.
 */
typedef struct {
  blkdev_t *parent; /**< The physical block device containing this partition. */
  uint32_t start_lba;    /**< Absolute LBA offset from the start of the physical
                            disk. */
  uint32_t sector_count; /**< Size of the partition in sectors. */
  int part_index;        /**< Primary partition index (0-3). */
} partition_info_t;

/**
 * @brief Scans a physical block device for an MBR and registers partitions.
 * * Reads the first sector of the device, verifies the boot signature (0xAA55),
 * and iterates through the partition table. Valid partitions are typically
 * registered as new virtual block devices (e.g., "ata0p1").
 * * @param physical_dev The physical device to probe.
 */
void partition_probe(blkdev_t *physical_dev);

#include "include/memory.h"
#include "include/stdio.h"
#include <stdbool.h>
#include <stdint.h>

extern uint8_t __bss_start;
extern uint8_t __end;

typedef struct DiskParams {
  uint8_t hdds_count;
  uint8_t drive_id;
  uint16_t cylinders;
  uint16_t sectors;
  uint16_t heads;
  bool lba_support;
} DiskParams;

typedef struct __attribute__((packed)) {
  uint8_t boot_flag;      // 0x80 = bootable, 0x00 = non-bootable
  uint8_t chs_start[3];   // CHS address of first sector (ignored by LBA)
  uint8_t partition_type; // 0x01=FAT12, 0x0B/F=FAT32, etc
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t total_sectors;
} MBRPartitionEntry;

void __attribute__((section(".entry")))
start(const MBRPartitionEntry *partition_table, int partitions_count,
      DiskParams disk_params) {

  memset(&__bss_start, 0, (&__end) - (&__bss_start));
  clear_screen();

  printf("MBR Partition Table:\n"
         "Idx | Boot | Type |   LBA Start  | Total Sectors\n"
         "-----------------------------------------------\n");

  for (int i = 0; i < partitions_count; i++) {
    MBRPartitionEntry p = partition_table[i];
    printf("%d |  0x%X | 0x%X | %u | %u\n", i, p.boot_flag, p.partition_type,
           p.lba_start, p.total_sectors);
  }

  printf("===== Disk Parameters =====\n"
         "Cylinders    : %u\n"
         "Heads        : %u\n"
         "Sectors/Track: %u\n"
         "Drive ID     : 0x%02X\n"
         "HDD Count    : %u\n"
         "LBA Support  : %s\n"
         "===========================\n",
         disk_params.cylinders, disk_params.heads, disk_params.sectors,
         disk_params.drive_id, disk_params.hdds_count,
         disk_params.lba_support ? "Yes" : "No");

  for (;;) {
  }
}

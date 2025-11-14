#include "arch/i686/vga_text.h"
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

typedef struct {
  uint64_t base;
  uint64_t length;
  uint32_t type;
  uint32_t acpi_flags;
  uint32_t reserved1;
  uint32_t reserved2;
} __attribute__((packed)) E820Entry;

void __attribute__((section(".entry")))
start(const MBRPartitionEntry *partition_table, int partitions_count,
      DiskParams disk_params, const E820Entry *memory_map,
      const int memory_map_entry_count) {

  memset(&__bss_start, 0, (&__end) - (&__bss_start));
  vga_clrscr();
  vga_setcursor(0, 0);

  debugf("MBR Partition Table:\n"
         "Idx | Boot | Type |   LBA Start  | Total Sectors\n"
         "-----------------------------------------------\n");

  for (int i = 0; i < partitions_count; i++) {
    MBRPartitionEntry p = partition_table[i];
    debugf("%d |  0x%X | 0x%X | %u | %u\n", i, p.boot_flag, p.partition_type,
           p.lba_start, p.total_sectors);
  }

  debugf("\nMemory map:\n"
         "Idx | Base  |  Length    | Type | ACPI\n"
         "-----------------------------------------------------------------\n");

  for (int i = 0; i < memory_map_entry_count; i++) {
    E820Entry *e = &memory_map[i];

    debugf("%d | %llu | %llu | %u | %u\n", i, e->base, e->length, e->type,
           e->acpi_flags);
  }

  debugf("\nDisk Parameters:\n"
         "====================\n"
         "Cylinders    : %u\n"
         "Heads        : %u\n"
         "Sectors/Track: %u\n"
         "Drive ID     : %X\n"
         "HDD Count    : %u\n"
         "LBA Support  : %s\n",
         disk_params.cylinders, disk_params.heads, disk_params.sectors,
         disk_params.drive_id, disk_params.hdds_count,
         disk_params.lba_support ? "Yes" : "No");

  for (;;) {
  }
}

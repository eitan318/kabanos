#include "memory_map.h"
#include "boot/bootparams.h"
#include "stdio.h"

MemoryRegion g_mem_regions[MAX_MEMORY_REGIONS];
int g_mem_region_count;

void memory_map_detect(MemoryMap *memory_map) {
  MemoryRegion block;
  uint32_t continuation = 0;
  int ret;

  g_mem_region_count = 0;
  ret = x86_e820_get_next_block(&block, &continuation);

  while (ret > 0 && continuation != 0) {
    g_mem_regions[g_mem_region_count].base = block.base;
    g_mem_regions[g_mem_region_count].length = block.length;
    g_mem_regions[g_mem_region_count].type = block.type;
    g_mem_regions[g_mem_region_count].acpi_flag = block.acpi_flag;
    ++g_mem_region_count;

    ret = x86_e820_get_next_block(&block, &continuation);
  }

  // fill meminfo structure
  memory_map->region_count = g_mem_region_count;
  memory_map->regions = g_mem_regions;
}

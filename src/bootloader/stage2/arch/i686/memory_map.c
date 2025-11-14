#include "memory_map.h"
#define MAX_E820_ENTRIES 32

E820Entry memory_map[MAX_E820_ENTRIES];

extern bool bios_fill_memory_map(E820Entry *buf, int max_entries);

bool memory_map_init() {
  return bios_fill_memory_map(memory_map, MAX_E820_ENTRIES);
}

E820Entry *memory_map_get() { return memory_map; }

int memory_map_count_get() {
  int i;
  for (i = 0; i < MAX_E820_ENTRIES; i++) {
    if (memory_map[i].type == 0)
      return i;
  }
  return i;
}

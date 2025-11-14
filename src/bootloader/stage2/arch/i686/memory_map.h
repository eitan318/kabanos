#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t base;
  uint64_t length;
  uint32_t type;
  uint32_t acpi_flags;
  uint32_t reserved1;
  uint32_t reserved2;
} __attribute__((packed)) E820Entry;

bool memory_map_init();
E820Entry *memory_map_get();
int memory_map_count_get();

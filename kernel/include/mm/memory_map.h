/**
 * @file memory_map.h
 * @brief Physical memory map (E820-style regions).
 */
#pragma once
#include "adt/range.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

/** @brief Region types as reported by the BIOS E820 interface. */
enum E820MemoryBlockType {
  E820_USABLE = 1,
  E820_RESERVED = 2,
  E820_ACPI_RECLAIMABLE = 3,
  E820_ACPI_NVS = 4,
  E820_BAD_MEMORY = 5,
};

/** @brief One physical memory region. */
typedef struct {
  uint64_t start;
  uint64_t size;
  uint32_t type; /**< An E820MemoryBlockType value. */
} memory_region_t;

/** @brief The full physical memory map. */
typedef struct {
  uint32_t region_count;
  memory_region_t *regions;
} memory_map_t;

#define MAX_USED_RANGES 64

/** @brief Fixed-capacity list of address ranges. */
typedef struct {
  range_t ranges[MAX_USED_RANGES];
  size_t count;
} range_list_t;

/** @brief Appends @p r to the list; silently drops it when full. */
static void range_list_push(range_list_t *list, range_t r) {
  if (list->count < MAX_USED_RANGES) {
    list->ranges[list->count++] = r;
  }
}

/** @brief Returns the range spanning from the lowest to the highest
 *         address in the map. */
range_t get_memory_range(memory_map_t *memory_map);

/** @brief Collects all non-usable regions (and gaps) within
 *         @p memory_range into @p list. */
void collect_non_usable_ranges(range_list_t *list, memory_map_t *memory_map,
                               range_t memory_range);

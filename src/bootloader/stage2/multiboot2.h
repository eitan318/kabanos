#include "memory_map.h"
#include <stdint.h>

void multiboot2_build(uint8_t *buffer, char *cmdline, int module_count,
                      void **modules_start, int *modules_size,
                      char **modules_paths, MemoryMap *memmap);

void multiboot2_jump_to_kernel(void *kernel_entry, uint8_t *multiboot2_info);

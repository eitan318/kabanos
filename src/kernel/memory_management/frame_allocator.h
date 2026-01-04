#pragma once

#include "boot/bootparams.h"
#include "utils/range.h"
#include <stdbool.h>

#include <stdint.h>

#define FRAME_SIZE 4096

void frame_allocator_init(BootParams *boot_params);
uint64_t frame_alloc();
void frame_free(uint64_t frame_addr);
void frame_mark_range_used(Range range);
uint64_t frame_get_free_count();

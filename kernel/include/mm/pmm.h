/**
 * @file pmm.h
 * @brief Physical memory manager: 4 KiB frame allocator with per-frame
 *        reference counts (used for COW sharing).
 */
#pragma once

#include "adt/range.h"

#define FRAME_SIZE 4096

typedef uint32_t paddr_t;

/**
 * @brief Initializes the frame allocator.
 * @param total_range Full physical address range to manage.
 * @param usable_ranges RAM ranges available for allocation.
 * @param critical_ranges Ranges that must never be handed out
 *        (kernel image, boot data, ...).
 */
void pmm_init(range_t total_range, range_t *usable_ranges,
              int usable_ranges_count, range_t *critical_ranges,
              int critical_ranges_count);

/** @brief Size of the allocator's bookkeeping for the given range,
 *         so callers can reserve room for it. */
uint64_t pmm_get_metadata_size(range_t total_memory_range);

/** @brief Allocates one frame; returns its physical address, or 0 if
 *         memory is exhausted. */
uint64_t pmm_frame_alloc();

/** @brief Drops one reference; frees the frame when the count hits zero. */
void pmm_frame_free(paddr_t frame_addr);

/** @brief Adds a reference to a frame (COW sharing). */
void pmm_frame_refcount_inc(paddr_t frame_addr);

/** @brief Returns the current reference count of a frame. */
uint16_t pmm_frame_refcount_get(paddr_t frame_addr);

uint64_t frame_get_free_count();
uint64_t frame_get_used_count();
uint64_t frame_get_total_count();

/** @brief Marks a physical range as allocated (unit tests only). */
void pmm_mark_range_used(paddr_t from, paddr_t to);

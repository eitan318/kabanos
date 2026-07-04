/**
 * @file kmalloc.h
 * @brief Kernel heap allocator.
 */
#pragma once
#include "klib/stddef.h"

/** @brief Initializes the kernel heap over the KERNEL_HEAP_* region. */
void kmalloc_init();

/** @brief Allocates @p size bytes; returns NULL on failure. */
void *kmalloc(size_t size);

/** @brief Like kmalloc(), but the memory is zero-initialized. */
void *kzalloc(size_t size);

/** @brief Frees a block returned by the k*alloc family; NULL is a no-op. */
void kfree(void *ptr);

/** @brief Resizes a block, preserving its contents. */
void *krealloc(void *ptr, size_t size);

/** @brief Allocates a zero-initialized array of @p count * @p size bytes. */
void *kcalloc(size_t count, size_t size);

/** @brief Allocator counters, for diagnostics. */
typedef struct {
  size_t total_allocated;    /**< Bytes allocated since boot. */
  size_t total_freed;        /**< Bytes freed since boot. */
  size_t current_usage;      /**< Live bytes right now. */
  size_t failed_allocations; /**< Number of allocations that returned NULL. */
} kmalloc_stats_t;

void kmalloc_stats_get(kmalloc_stats_t *stats);

#pragma once
#include "hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kmalloc_init();
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);
void *kcalloc(size_t count, size_t size);

typedef struct {
  size_t total_allocated;
  size_t total_freed;
  size_t current_usage;
  size_t failed_allocations;
} kmalloc_stats_t;

void kmalloc_stats_get(kmalloc_stats_t *stats);

/**
 * @file va_allocation.h
 * @brief Backing a virtual region with freshly allocated physical frames.
 */
#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

#include "arch/types.h"

/**
 * @brief Allocates physical frames and maps them at [virt_start,
 *        virt_start + size) with the given PAGE_* flags.
 * @return true on success; already-mapped or out-of-memory pages fail.
 */
bool va_alloc_region(arch_vm_t *vm, uint32_t virt_start, size_t size,
                     uint32_t flags);

/** @brief Unmaps the region and releases its physical frames. */
void va_free_region(arch_vm_t *vm, uint32_t virt_start, size_t size);

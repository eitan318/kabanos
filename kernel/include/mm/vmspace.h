/**
 * @file vmspace.h
 * @brief Virtual Memory Space management.
 */

#pragma once
#include "adt/range.h"
#include "arch/types.h"
#include "klib/stddef.h"

/** @brief Container for virtual memory metadata. */
typedef struct vmspace_t {
  arch_vm_t *arch; /**< Hardware page table pointer. */
} vmspace_t;

/**
 * @brief Initializes kernel address space with higher-half mappings.
 * @param vmspace Destination vmspace.
 * @param total_memory_range Physical range to map.
 */
void kernel_vmspace_create(vmspace_t *vmspace, range_t total_memory_range);

/**
 * @brief Creates a new user vmspace with kernel mappings mirrored.
 * @return New vmspace_t pointer, or NULL on failure.
 */
vmspace_t *vmspace_create();

/**
 * @brief Clones an existing vmspace (for fork).
 * @param other Source vmspace to duplicate.
 * @return Cloned vmspace_t pointer, or NULL on failure.
 */
vmspace_t *vmspace_clone(vmspace_t *other);

/**
 * @brief Activates the given address space in hardware.
 * @param vmspace Space to switch to.
 */
void vmspace_switch(vmspace_t *vmspace);

/**
 * @brief Frees vmspace and underlying architecture structures.
 * @param vmspace Space to destroy.
 */
void vmspace_destroy(vmspace_t *vmspace);

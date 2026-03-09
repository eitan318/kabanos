/**
 * @file vmspace.h
 * @brief Virtual Memory Space management.
 */

#pragma once
#include "adt/range.h"
#include "arch/types.h"
#include "hal.h"
#include "klib/stddef.h"

/** @brief a virtual memory area */
typedef struct vma {
  range_t range;  /**< the memory range of the area */
  uint32_t flags; /**< area properties e.g.: RW COW AOR */

  struct vma *next; /**< linking for za */
} vma_t;

/** @brief Container for virtual memory metadata. */
typedef struct {
  arch_vm_t *arch; /**< Hardware page table pointer. */
  vma_t *vma_list;
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

/**
 * @brief adds a vma
 * @note in the future should become vmspace_add_vma_sorted,
 * and find should be using binary search
 */
void vmspace_add_vma(vmspace_t *vmspace, vma_t *new_vma);

/**
 * @brief find a virtual memory area based on an address in the area
 * @note in the future should be using binary search
 */
vma_t *vmspace_find_vma(vmspace_t *vmspace, vaddr_t addr);

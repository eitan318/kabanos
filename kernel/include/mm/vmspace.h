/**
 * @file vmspace.h
 * @brief Virtual Memory Space management.
 */

#pragma once
#include "adt/range.h"
#include "arch/types.h"
#include "hal.h"
#include "klib/stddef.h"

typedef enum {
  VMA_READ = (1 << 0),
  VMA_WRITE = (1 << 1),
  VMA_EXECUTE = (1 << 2),
  VMA_USER = (1 << 3),
  VMA_STACK = (1 << 4),     // Software only: identifies the stack
  VMA_ANONYMOUS = (1 << 5), // Software only: no file backing
  VMA_COW = (1 << 6),       // Software only: copy-on-write active
  VMA_HEAP = (1 << 7),
} vma_flags_t;

/** @brief a virtual memory area */
typedef struct vma {
  range_t range;  /**< the memory range of the area */
  uint32_t flags; /**< area properties e.g.: RW COW AOR */

  struct vma *next; /**< linking for za */
} vma_t;

/**
 * @brief constructs a vma from params
 */
vma_t *vma_create(vaddr_t va_start, size_t mem_size, uint32_t flags);

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

/**
 * @brief alloc a user stack
 */
bool vmspace_map_stack(vmspace_t *vm, uint32_t stack_top, size_t size);

/**
 * @brief alloc a user heap
 */
bool vmspace_map_heap(vmspace_t *vm, uint32_t heap_start, size_t initial_size);

/**
 * @brief extend a user heap
 */
bool vmspace_extend_heap(vmspace_t *vm, uint32_t old_brk, uint32_t new_brk);

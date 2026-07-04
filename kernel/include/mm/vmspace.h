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
  VMA_STACK = (1 << 4),     /**< Software only: identifies the stack. */
  VMA_ANONYMOUS = (1 << 5), /**< Software only: no file backing. */
  VMA_COW = (1 << 6),       /**< Software only: copy-on-write active. */
  VMA_HEAP = (1 << 7),      /**< Software only: identifies the heap. */
} vma_flags_t;

/** @brief A virtual memory area: one contiguous mapped region. */
typedef struct vma {
  range_t range;  /**< Virtual address range of the area. */
  uint32_t flags; /**< Combination of vma_flags_t bits. */

  struct vma *next; /**< Next area in the vmspace's list. */
} vma_t;

/** @brief Allocates a vma describing [va_start, va_start + mem_size). */
vma_t *vma_create(vaddr_t va_start, size_t mem_size, uint32_t flags);

/** @brief A full address space: hardware page tables plus the vma list. */
typedef struct {
  arch_vm_t *arch; /**< Hardware page table pointer. */
  vma_t *vma_list; /**< Mapped areas, as a linked list. */

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
 * @brief Adds a vma to the space's list.
 * @note Should eventually insert sorted so vmspace_find_vma can use
 *       binary search.
 */
void vmspace_add_vma(vmspace_t *vmspace, vma_t *new_vma);

/** @brief Finds the vma containing @p addr, or NULL if unmapped. */
vma_t *vmspace_find_vma(vmspace_t *vmspace, vaddr_t addr);

/** @brief Maps and registers a user stack ending at @p stack_top. */
bool vmspace_map_stack(vmspace_t *vm, uint32_t stack_top, size_t size);

/** @brief Maps and registers a user heap starting at @p heap_start. */
bool vmspace_map_heap(vmspace_t *vm, uint32_t heap_start, size_t initial_size);

/** @brief Grows the heap vma and its mappings from @p old_brk to
 *         @p new_brk. */
bool vmspace_extend_heap(vmspace_t *vm, uint32_t old_brk, uint32_t new_brk);

/** @brief Copies @p size bytes from @p src_vmspace into the current
 *         address space. */
int vmspace_copy_from(arch_vm_t *src_vmspace, vaddr_t dst_vaddr,
                      vaddr_t src_vaddr, size_t size);

/** @brief Copies @p size bytes from the current address space into
 *         @p dst_vmspace. */
int vmspace_copy_to(arch_vm_t *dst_vmspace, vaddr_t dst_vaddr,
                    vaddr_t src_vaddr, size_t size);

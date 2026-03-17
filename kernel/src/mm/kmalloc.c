#include "mm/kmalloc.h"
#include "arch/types.h"
#include "hal.h"
#include "klib/string.h"
#include "mm/memdefs.h"
#include "mm/pmm.h"
#include "mm/vmspace.h"

// Size classes for slab allocator (in bytes)
static const size_t SIZE_CLASSES[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
#define NUM_SIZE_CLASSES (sizeof(SIZE_CLASSES) / sizeof(SIZE_CLASSES[0]))

// Each slab contains multiple objects of the same size
typedef struct slab {
  struct slab *next;    // Next slab in the list
  void *free_list;      // Free list of objects in this slab
  uint32_t free_count;  // Number of free objects
  uint32_t total_count; // Total objects in slab
  size_t object_size;   // Size of each object
} slab_t;

// Cache for each size class
typedef struct {
  slab_t *partial_slabs;     // Slabs with some free objects
  slab_t *full_slabs;        // Slabs with no free objects
  slab_t *empty_slabs;       // Slabs with all free objects
  size_t object_size;        // Size of objects in this cache
  uint32_t objects_per_slab; // Number of objects per slab
} kmem_cache_t;

// Global caches for each size class
static kmem_cache_t caches[NUM_SIZE_CLASSES];

// Statistics
static kmalloc_stats_t stats = {0};

static uint32_t next_heap_addr = KERNEL_HEAP_START;

arch_vm_t *kernel_arch_vm;

/**
 * Allocate a page from the kernel heap
 */
static void *heap_page_alloc(void) {
  if (next_heap_addr >= KERNEL_HEAP_END) {
    return NULL;
  }

  // Allocate physical frame
  uint32_t physical = pmm_frame_alloc();
  if (physical == 0) {
    return NULL;
  }

  // Map it to virtual address
  uint32_t virtual = next_heap_addr;
  if (!hal_vm_map(kernel_arch_vm, virtual, physical, PAGE_READWRITE)) {
    pmm_frame_free(physical);
    return NULL;
  }

  next_heap_addr += PAGE_SIZE;
  return (void *)virtual;
}

/**
 * Free a page back to the heap
 */
static void heap_page_free(void *ptr) {
  uint32_t virtual = (uint32_t)ptr;
  uint32_t physical = hal_vm_virt_to_phys(kernel_arch_vm, virtual);

  if (physical) {
    hal_vm_unmap(kernel_arch_vm, virtual);
    pmm_frame_free(physical);
  }
}
/**
 * Create a new slab for the given cache
 */
static slab_t *slab_create(kmem_cache_t *cache) {
  // Allocate page for slab
  void *page = heap_page_alloc();
  if (!page) {
    return NULL;
  }

  // Slab header is at the beginning of the page
  slab_t *slab = (slab_t *)page;
  slab->next = NULL;
  slab->object_size = cache->object_size;

  // Calculate how many objects fit after the slab header
  size_t header_size = sizeof(slab_t);
  size_t available = PAGE_SIZE - header_size;
  slab->total_count = available / cache->object_size;
  slab->free_count = slab->total_count;

  // Build free list
  uint8_t *obj = (uint8_t *)page + header_size;
  void **free_list = NULL;

  for (uint32_t i = 0; i < slab->total_count; i++) {
    void **next_ptr = (void **)obj;
    *next_ptr = free_list;
    free_list = (void *)obj;
    obj += cache->object_size;
  }

  slab->free_list = free_list;
  return slab;
}

/**
 * Destroy a slab
 */
static void slab_destroy(slab_t *slab) { heap_page_free(slab); }

/**
 * Allocate an object from a slab
 */
static void *slab_alloc(slab_t *slab) {
  if (!slab->free_list) {
    return NULL;
  }

  // Pop from free list
  void *obj = slab->free_list;
  slab->free_list = *(void **)obj;
  slab->free_count--;

  return obj;
}

/**
 * Free an object back to a slab
 */
static void slab_free(slab_t *slab, void *ptr) {
  // Push onto free list
  *(void **)ptr = slab->free_list;
  slab->free_list = ptr;
  slab->free_count++;
}

/**
 * Find which slab an address belongs to
 */
static slab_t *slab_find(void *ptr) {
  // Slabs are page-aligned
  uint32_t addr = (uint32_t)ptr;
  return (slab_t *)(addr & ~(PAGE_SIZE - 1));
}

/**
 * Get cache for a given size
 */
static kmem_cache_t *cache_for_size(size_t size) {
  for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
    if (size <= SIZE_CLASSES[i]) {
      return &caches[i];
    }
  }
  return NULL;
}

/**
 * Initialize the kernel memory allocator
 */

void kmalloc_init() {
  // Initialize each cache
  for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
    caches[i].partial_slabs = NULL;
    caches[i].full_slabs = NULL;
    caches[i].empty_slabs = NULL;
    caches[i].object_size = SIZE_CLASSES[i];

    // Calculate objects per slab
    size_t available = PAGE_SIZE - sizeof(slab_t);
    caches[i].objects_per_slab = available / SIZE_CLASSES[i];
  }

  extern vmspace_t *g_kernel_vmspace;
  kernel_arch_vm = g_kernel_vmspace->arch;
}

/**
 * Internal allocation function
 */
static void *kmalloc_internal(size_t size, bool zero) {
  if (size == 0) {
    return NULL;
  }

  // For large allocations, use direct page allocation
  if (size > SIZE_CLASSES[NUM_SIZE_CLASSES - 1]) {
    // Round up to page size
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void *ptr = NULL;

    for (size_t i = 0; i < pages; i++) {
      void *page = heap_page_alloc();
      if (!page) {
        // TODO: Free previously allocated pages
        stats.failed_allocations++;
        return NULL;
      }
      if (i == 0) {
        ptr = page;
      }
    }

    stats.total_allocated += pages * PAGE_SIZE;
    stats.current_usage += pages * PAGE_SIZE;

    if (zero) {
      memset(ptr, 0, pages * PAGE_SIZE);
    }

    return ptr;
  }

  // Find appropriate cache
  kmem_cache_t *cache = cache_for_size(size);
  if (!cache) {
    stats.failed_allocations++;
    return NULL;
  }

  // Try partial slabs first
  slab_t *slab = cache->partial_slabs;
  if (!slab) {
    // Try empty slabs
    slab = cache->empty_slabs;
    if (slab) {
      // Move from empty to partial
      cache->empty_slabs = slab->next;
      slab->next = cache->partial_slabs;
      cache->partial_slabs = slab;
    }
  }

  // No slabs available, create new one
  if (!slab) {
    slab = slab_create(cache);
    if (!slab) {
      stats.failed_allocations++;
      return NULL;
    }
    slab->next = cache->partial_slabs;
    cache->partial_slabs = slab;
  }

  // Allocate from slab
  void *ptr = slab_alloc(slab);
  if (!ptr) {
    stats.failed_allocations++;
    return NULL;
  }

  // If slab is now full, move it to full list
  if (slab->free_count == 0) {
    // Remove from partial list
    if (cache->partial_slabs == slab) {
      cache->partial_slabs = slab->next;
    }
    // Add to full list
    slab->next = cache->full_slabs;
    cache->full_slabs = slab;
  }

  stats.total_allocated += cache->object_size;
  stats.current_usage += cache->object_size;

  if (zero) {
    memset(ptr, 0, cache->object_size);
  }

  return ptr;
}

/**
 * Allocate kernel memory
 */
void *kmalloc(size_t size) { return kmalloc_internal(size, false); }

/**
 * Allocate and zero kernel memory
 */
void *kzalloc(size_t size) { return kmalloc_internal(size, true); }

/**
 * Allocate memory like calloc(count, size)
 */
void *kcalloc(size_t count, size_t size) {
  // Check for overflow
  if (count != 0 && size > SIZE_MAX / count) {
    stats.failed_allocations++;
    return NULL;
  }
  return kzalloc(count * size);
}

/**
 * Free kernel memory
 */
void kfree(void *ptr) {
  if (!ptr) {
    return;
  }

  // Find the slab this belongs to
  slab_t *slab = slab_find(ptr);
  size_t obj_size = slab->object_size;

  // Find the cache
  kmem_cache_t *cache = cache_for_size(obj_size);
  if (!cache) {
    // Large allocation - free pages directly
    heap_page_free(ptr);
    stats.total_freed += PAGE_SIZE;
    stats.current_usage -= PAGE_SIZE;
    return;
  }

  // Was this slab full?
  bool was_full = (slab->free_count == 0);

  // Free the object
  slab_free(slab, ptr);
  stats.total_freed += obj_size;
  stats.current_usage -= obj_size;

  // If slab was full, move to partial list
  if (was_full) {
    slab_t **prev = &cache->full_slabs;
    while (*prev && *prev != slab) {
      prev = &(*prev)->next;
    }
    if (*prev) { // Only unlink if we actually found it!
      *prev = slab->next;
      // Add to partial list
      slab->next = cache->partial_slabs;
      cache->partial_slabs = slab;
    }
  }

  // If slab is now empty, consider moving to empty list
  if (slab->free_count == slab->total_count) {
    // Remove from partial list
    slab_t **prev = &cache->partial_slabs;
    while (*prev && *prev != slab) {
      prev = &(*prev)->next;
    }
    if (*prev == slab) {
      *prev = slab->next;
    }

    // Add to empty list
    slab->next = cache->empty_slabs;
    cache->empty_slabs = slab;

    // Optional: Destroy empty slabs to reclaim memory
    // slab_destroy(slab);
  }
}

/**
 * Reallocate kernel memory
 */
void *krealloc(void *ptr, size_t size) {
  if (!ptr) {
    return kmalloc(size);
  }

  if (size == 0) {
    kfree(ptr);
    return NULL;
  }

  // Find current slab
  slab_t *slab = slab_find(ptr);
  size_t old_size = slab->object_size;

  // If new size fits in same size class, just return
  kmem_cache_t *old_cache = cache_for_size(old_size);
  kmem_cache_t *new_cache = cache_for_size(size);

  if (old_cache == new_cache) {
    return ptr;
  }

  // Allocate new block
  void *new_ptr = kmalloc(size);
  if (!new_ptr) {
    return NULL;
  }

  // Copy old data
  size_t copy_size = (old_size < size) ? old_size : size;
  memcpy(new_ptr, ptr, copy_size);

  // Free old block
  kfree(ptr);

  return new_ptr;
}

/**
 * Get statistics
 */
void kmalloc_stats_get(kmalloc_stats_t *out) { *out = stats; }

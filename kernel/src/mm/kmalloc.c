/**
 * @file kmalloc.c
 * @brief Slab-based kernel heap allocator.
 *
 * Small allocations come from per-size-class slab caches (one page per
 * slab); allocations larger than the biggest size class fall back to
 * direct page allocation.
 */
#include "mm/kmalloc.h"
#include "arch/types.h"
#include "hal.h"
#include "klib/stdio.h"
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

// Allocations above the largest size class get whole pages, prefixed by
// this header so kfree/krealloc can recover the allocation's page count.
typedef struct {
  uint32_t magic;
  uint32_t pages;
  uint32_t reserved[2]; // keeps returned pointers 16-byte aligned
} large_alloc_hdr_t;

// Can never collide with a slab page: the slab header's first word is a
// pointer into the heap (>= KERNEL_HEAP_START) or NULL.
#define LARGE_ALLOC_MAGIC 0x4C41524B

// Global caches for each size class
static kmem_cache_t caches[NUM_SIZE_CLASSES];

// Statistics
static kmalloc_stats_t stats = {0};

arch_vm_t *kernel_arch_vm;

//
// Heap virtual address allocator: one bit per heap page, first fit.
// Freed ranges become reusable, so the heap's address space no longer
// leaks (the old bump pointer burned VA on every allocation forever).
//

#define HEAP_PAGES (KERNEL_HEAP_SIZE / PAGE_SIZE)
static uint8_t heap_va_bitmap[HEAP_PAGES / 8];
static size_t heap_va_hint = 0;

static inline bool heap_va_test(size_t page) {
  return heap_va_bitmap[page >> 3] & (1u << (page & 7));
}

static inline void heap_va_set(size_t page) {
  heap_va_bitmap[page >> 3] |= (1u << (page & 7));
}

static inline void heap_va_clear(size_t page) {
  heap_va_bitmap[page >> 3] &= ~(1u << (page & 7));
}

/** @brief Reserves @p page_count contiguous heap virtual pages. */
static uint32_t heap_va_alloc(size_t page_count) {
  if (page_count == 0 || page_count > HEAP_PAGES) {
    return 0;
  }

  // Two passes: hint..end, then 0..hint. A run never spans the seam.
  for (int pass = 0; pass < 2; pass++) {
    size_t lo = pass ? 0 : heap_va_hint;
    size_t hi = pass ? heap_va_hint : HEAP_PAGES;
    size_t run = 0;

    for (size_t i = lo; i < hi; i++) {
      if (heap_va_test(i)) {
        run = 0;
        continue;
      }
      if (++run < page_count) {
        continue;
      }

      size_t first = i + 1 - page_count;
      for (size_t j = first; j <= i; j++) {
        heap_va_set(j);
      }
      heap_va_hint = (i + 1 < HEAP_PAGES) ? i + 1 : 0;
      return KERNEL_HEAP_START + first * PAGE_SIZE;
    }
  }
  return 0;
}

static void heap_va_free(uint32_t va, size_t page_count) {
  size_t first = (va - KERNEL_HEAP_START) / PAGE_SIZE;
  for (size_t i = 0; i < page_count; i++) {
    heap_va_clear(first + i);
  }
  if (first < heap_va_hint) {
    heap_va_hint = first;
  }
}

/** @brief Allocate a page from the kernel heap. */
static void *heap_page_alloc(void) {
  uint32_t virtual = heap_va_alloc(1);
  if (!virtual) {
    return NULL;
  }

  uint32_t physical = pmm_frame_alloc();
  if (physical == 0) {
    heap_va_free(virtual, 1);
    return NULL;
  }

  if (!hal_vm_map(kernel_arch_vm, virtual, physical, PAGE_READWRITE)) {
    pmm_frame_free(physical);
    heap_va_free(virtual, 1);
    return NULL;
  }

  return (void *)virtual;
}

/** @brief Free a page back to the heap. */
static void heap_page_free(void *ptr) {
  uint32_t virtual = (uint32_t)ptr;
  uint32_t physical = hal_vm_virt_to_phys(kernel_arch_vm, virtual);

  if (physical) {
    hal_vm_unmap(kernel_arch_vm, virtual);
    pmm_frame_free(physical);
  }
  heap_va_free(virtual, 1);
}
/** @brief Create a new slab for the given cache. */
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

/** @brief Destroy a slab. */
static void slab_destroy(slab_t *slab) { heap_page_free(slab); }

/** @brief Allocate an object from a slab. */
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

/** @brief Free an object back to a slab. */
static void slab_free(slab_t *slab, void *ptr) {
  // Push onto free list
  *(void **)ptr = slab->free_list;
  slab->free_list = ptr;
  slab->free_count++;
}

/** @brief Find which slab an address belongs to. */
static slab_t *slab_find(void *ptr) {
  // Slabs are page-aligned
  uint32_t addr = (uint32_t)ptr;
  return (slab_t *)(addr & ~(PAGE_SIZE - 1));
}

/** @brief Get cache for a given size. */
static kmem_cache_t *cache_for_size(size_t size) {
  for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
    if (size <= SIZE_CLASSES[i]) {
      return &caches[i];
    }
  }
  return NULL;
}

/** @brief Initialize the kernel memory allocator. */
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

/** @brief Internal allocation function. */
static void *kmalloc_internal(size_t size, bool zero) {
  if (size == 0) {
    return NULL;
  }

  // For large allocations, use direct page allocation
  if (size > SIZE_CLASSES[NUM_SIZE_CLASSES - 1]) {
    size_t pages =
        (size + sizeof(large_alloc_hdr_t) + PAGE_SIZE - 1) / PAGE_SIZE;

    uint32_t va = heap_va_alloc(pages);
    if (!va) {
      stats.failed_allocations++;
      return NULL;
    }

    size_t mapped;
    for (mapped = 0; mapped < pages; mapped++) {
      uint32_t phys = pmm_frame_alloc();
      if (!phys) {
        break;
      }
      if (!hal_vm_map(kernel_arch_vm, va + mapped * PAGE_SIZE, phys,
                      PAGE_READWRITE)) {
        pmm_frame_free(phys);
        break;
      }
    }

    if (mapped < pages) {
      // Partial failure: roll everything back
      for (size_t i = 0; i < mapped; i++) {
        uint32_t v = va + i * PAGE_SIZE;
        uint32_t phys = hal_vm_virt_to_phys(kernel_arch_vm, v);
        hal_vm_unmap(kernel_arch_vm, v);
        pmm_frame_free(phys);
      }
      heap_va_free(va, pages);
      stats.failed_allocations++;
      return NULL;
    }

    large_alloc_hdr_t *hdr = (large_alloc_hdr_t *)va;
    hdr->magic = LARGE_ALLOC_MAGIC;
    hdr->pages = pages;

    stats.total_allocated += pages * PAGE_SIZE;
    stats.current_usage += pages * PAGE_SIZE;

    void *ptr = (void *)(va + sizeof(*hdr));
    if (zero) {
      memset(ptr, 0, pages * PAGE_SIZE - sizeof(*hdr));
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

/** @brief Allocate kernel memory. */
void *kmalloc(size_t size) { return kmalloc_internal(size, false); }

/** @brief Allocate and zero kernel memory. */
void *kzalloc(size_t size) { return kmalloc_internal(size, true); }

/** @brief Allocate memory like calloc(count, size). */
void *kcalloc(size_t count, size_t size) {
  // Check for overflow
  if (count != 0 && size > SIZE_MAX / count) {
    stats.failed_allocations++;
    return NULL;
  }
  return kzalloc(count * size);
}

/** @brief Returns the large-allocation header for @p ptr, or NULL if it
 *         is a slab object. Slab objects always sit past the slab_t
 *         header, so they can never sit exactly sizeof(hdr) into a page,
 *         and the magic can never appear as a slab's first word. */
static large_alloc_hdr_t *large_alloc_hdr(void *ptr) {
  uint32_t page_start = (uint32_t)ptr & ~(PAGE_SIZE - 1);
  large_alloc_hdr_t *hdr = (large_alloc_hdr_t *)page_start;

  if ((uint32_t)ptr == page_start + sizeof(*hdr) &&
      hdr->magic == LARGE_ALLOC_MAGIC) {
    return hdr;
  }
  return NULL;
}

/** @brief Free kernel memory. */
void kfree(void *ptr) {
  if (!ptr) {
    return;
  }

  large_alloc_hdr_t *hdr = large_alloc_hdr(ptr);
  if (hdr) {
    uint32_t va = (uint32_t)hdr;
    size_t pages = hdr->pages;

    for (size_t i = 0; i < pages; i++) {
      uint32_t v = va + i * PAGE_SIZE;
      uint32_t phys = hal_vm_virt_to_phys(kernel_arch_vm, v);
      hal_vm_unmap(kernel_arch_vm, v);
      pmm_frame_free(phys);
    }
    heap_va_free(va, pages);

    stats.total_freed += pages * PAGE_SIZE;
    stats.current_usage -= pages * PAGE_SIZE;
    return;
  }

  // Find the slab this belongs to
  slab_t *slab = slab_find(ptr);
  size_t obj_size = slab->object_size;

  // Find the cache
  kmem_cache_t *cache = cache_for_size(obj_size);
  if (!cache) {
    kdebugf("kfree: bad pointer %p\n", ptr);
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

/** @brief Reallocate kernel memory. */
void *krealloc(void *ptr, size_t size) {
  if (!ptr) {
    return kmalloc(size);
  }

  if (size == 0) {
    kfree(ptr);
    return NULL;
  }

  size_t old_size;
  large_alloc_hdr_t *hdr = large_alloc_hdr(ptr);
  if (hdr) {
    old_size = hdr->pages * PAGE_SIZE - sizeof(*hdr);
    // Still a large allocation that fits in the pages we already have
    if (size > SIZE_CLASSES[NUM_SIZE_CLASSES - 1] && size <= old_size) {
      return ptr;
    }
  } else {
    old_size = slab_find(ptr)->object_size;
    // If new size fits in same size class, just return
    if (cache_for_size(old_size) == cache_for_size(size)) {
      return ptr;
    }
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

/** @brief Get statistics. */
void kmalloc_stats_get(kmalloc_stats_t *out) { *out = stats; }

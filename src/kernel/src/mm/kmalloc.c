#include "mm/kmalloc.h"
#include "arch/types.h"
#include "klib/string.h"
#include "mm/memdefs.h"
#include "mm/pmm.h"
#include "mm/vmspace.h"

// Size classes for slab allocator (in bytes)
static const size_t SIZE_CLASSES[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
//                                                                       ^^^^
//                                                                       remove
//                                                                       4096
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

// -----------------------------------------------------------------------
// FIX 1: Replace the bump-pointer heap with a page free-list so that
//         heap_page_free() can actually return pages for reuse instead of
//         leaking virtual address space.
// -----------------------------------------------------------------------
typedef struct free_page {
  struct free_page *next;
} free_page_t;

static uint32_t next_heap_addr = KERNEL_HEAP_START;
static free_page_t *free_page_list = NULL; // recycled pages

arch_vm_t *kernel_arch_vm;

/**
 * Allocate a page from the kernel heap (reuse freed pages first).
 */
static void *heap_page_alloc(void) {
  // FIX 1a: reuse a previously freed page before bumping the pointer
  if (free_page_list) {
    free_page_t *page = free_page_list;
    free_page_list = page->next;
    return (void *)page;
  }

  if (next_heap_addr >= KERNEL_HEAP_END) {
    return NULL;
  }

  uint32_t physical = pmm_frame_alloc();
  if (physical == 0) {
    return NULL;
  }

  uint32_t virt = next_heap_addr;
  if (!hal_vm_map(kernel_arch_vm, virt, physical, PAGE_READWRITE)) {
    pmm_frame_free(physical);
    return NULL;
  }

  next_heap_addr += PAGE_SIZE;
  return (void *)virt;
}

/**
 * Free a page back to the heap free-list for reuse.
 */
static void heap_page_free(void *ptr) {
  // FIX 1b: instead of unmapping (which makes the VA unusable forever),
  //         keep the mapping and push the page onto the free list.
  free_page_t *page = (free_page_t *)ptr;
  page->next = free_page_list;
  free_page_list = page;
}

// -----------------------------------------------------------------------
// FIX 2: Large-allocation header so we know the page count on kfree.
//
//   [ large_hdr_t | ... data ... ]   all within the first page, or the
//   header occupies the first PAGE_SIZE-aligned slot and data follows.
//   We store the header in a separate small slab allocation to keep the
//   data pointer page-aligned for callers that care.
// -----------------------------------------------------------------------
typedef struct {
  size_t page_count; // how many contiguous virtual pages were allocated
} large_hdr_t;

// We keep a tiny linked list of large allocation records in slab memory.
typedef struct large_alloc {
  struct large_alloc *next;
  void *ptr; // base virtual address (page-aligned)
  size_t page_count;
} large_alloc_t;

static large_alloc_t *large_alloc_list = NULL;

static void large_alloc_record(void *ptr, size_t page_count) {
  // Use the smallest slab class for the record itself
  large_alloc_t *rec = kmalloc(sizeof(large_alloc_t));
  if (!rec)
    return; // stats will be off but we won't crash
  rec->ptr = ptr;
  rec->page_count = page_count;
  rec->next = large_alloc_list;
  large_alloc_list = rec;
}

static size_t large_alloc_remove(void *ptr) {
  large_alloc_t **prev = &large_alloc_list;
  while (*prev) {
    if ((*prev)->ptr == ptr) {
      large_alloc_t *rec = *prev;
      size_t pages = rec->page_count;
      *prev = rec->next;
      kfree(rec); // frees the record back to the slab
      return pages;
    }
    prev = &(*prev)->next;
  }
  return 0; // not found — caller should treat as single page
}

/**
 * Create a new slab for the given cache
 */
static slab_t *slab_create(kmem_cache_t *cache) {
  void *page = heap_page_alloc();
  if (!page)
    return NULL;

  slab_t *slab = (slab_t *)page;
  slab->next = NULL;
  slab->object_size = cache->object_size;

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

static void slab_destroy(slab_t *slab) { heap_page_free(slab); }

static void *slab_alloc(slab_t *slab) {
  if (!slab->free_list)
    return NULL;

  void *obj = slab->free_list;
  slab->free_list = *(void **)obj;
  slab->free_count--;
  return obj;
}

static void slab_free(slab_t *slab, void *ptr) {
  *(void **)ptr = slab->free_list;
  slab->free_list = ptr;
  slab->free_count++;
}

static slab_t *slab_find(void *ptr) {
  uint32_t addr = (uint32_t)ptr;
  return (slab_t *)(addr & ~(PAGE_SIZE - 1));
}

static kmem_cache_t *cache_for_size(size_t size) {
  for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
    if (size <= SIZE_CLASSES[i])
      return &caches[i];
  }
  return NULL;
}

void kmalloc_init(void) {
  for (size_t i = 0; i < NUM_SIZE_CLASSES; i++) {
    caches[i].partial_slabs = NULL;
    caches[i].full_slabs = NULL;
    caches[i].empty_slabs = NULL;
    caches[i].object_size = SIZE_CLASSES[i];
    size_t available = PAGE_SIZE - sizeof(slab_t);
    caches[i].objects_per_slab = available / SIZE_CLASSES[i];
  }
  extern vmspace_t *g_kernel_vmspace;
  kernel_arch_vm = g_kernel_vmspace->arch;
}

static void *kmalloc_internal(size_t size, bool zero) {
  if (size == 0)
    return NULL;

  // Large allocation path
  if (size > SIZE_CLASSES[NUM_SIZE_CLASSES - 1]) {
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void *base = NULL;

    for (size_t i = 0; i < pages; i++) {
      void *page = heap_page_alloc();
      if (!page) {
        // FIX 2a: free every page we already got before giving up
        // We walk back through the free list — pages were pushed in order
        // so the last 'i' entries on free_page_list are ours.
        for (size_t j = 0; j < i; j++) {
          free_page_t *p = free_page_list;
          free_page_list = p->next;
          // Actually unmap + free physical for pages we haven't used yet
          uint32_t virt = (uint32_t)p;
          uint32_t phys = hal_vm_virt_to_phys(kernel_arch_vm, virt);
          if (phys) {
            hal_vm_unmap(kernel_arch_vm, virt);
            pmm_frame_free(phys);
          }
        }
        stats.failed_allocations++;
        return NULL;
      }
      if (i == 0)
        base = page;
    }

    // FIX 2b: record page count so kfree can free all pages
    large_alloc_record(base, pages);

    stats.total_allocated += pages * PAGE_SIZE;
    stats.current_usage += pages * PAGE_SIZE;
    if (zero)
      memset(base, 0, pages * PAGE_SIZE);
    return base;
  }

  // Slab path
  kmem_cache_t *cache = cache_for_size(size);
  if (!cache) {
    stats.failed_allocations++;
    return NULL;
  }

  slab_t *slab = cache->partial_slabs;
  if (!slab) {
    slab = cache->empty_slabs;
    if (slab) {
      cache->empty_slabs = slab->next;
      slab->next = cache->partial_slabs;
      cache->partial_slabs = slab;
    }
  }
  if (!slab) {
    slab = slab_create(cache);
    if (!slab) {
      stats.failed_allocations++;
      return NULL;
    }
    slab->next = cache->partial_slabs;
    cache->partial_slabs = slab;
  }

  void *ptr = slab_alloc(slab);
  if (!ptr) {
    stats.failed_allocations++;
    return NULL;
  }

  if (slab->free_count == 0) {
    if (cache->partial_slabs == slab)
      cache->partial_slabs = slab->next;
    slab->next = cache->full_slabs;
    cache->full_slabs = slab;
  }

  stats.total_allocated += cache->object_size;
  stats.current_usage += cache->object_size;
  if (zero)
    memset(ptr, 0, cache->object_size);
  return ptr;
}

void *kmalloc(size_t size) { return kmalloc_internal(size, false); }
void *kzalloc(size_t size) { return kmalloc_internal(size, true); }

void *kcalloc(size_t count, size_t size) {
  if (count != 0 && size > SIZE_MAX / count) {
    stats.failed_allocations++;
    return NULL;
  }
  return kzalloc(count * size);
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  slab_t *slab = slab_find(ptr);
  size_t obj_size = slab->object_size;

  kmem_cache_t *cache = cache_for_size(obj_size);
  if (!cache) {
    // FIX 2c: large allocation — look up page count and free all pages
    size_t page_count = large_alloc_remove(ptr);
    if (page_count == 0)
      page_count = 1; // safe fallback

    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < page_count; i++) {
      heap_page_free(p + i * PAGE_SIZE);
    }
    stats.total_freed += page_count * PAGE_SIZE;
    stats.current_usage -= page_count * PAGE_SIZE;
    return;
  }

  bool was_full = (slab->free_count == 0);
  slab_free(slab, ptr);
  stats.total_freed += obj_size;
  stats.current_usage -= obj_size;

  if (was_full) {
    slab_t **prev = &cache->full_slabs;
    while (*prev && *prev != slab)
      prev = &(*prev)->next;
    if (*prev == slab)
      *prev = slab->next;
    slab->next = cache->partial_slabs;
    cache->partial_slabs = slab;
  }

  if (slab->free_count == slab->total_count) {
    slab_t **prev = &cache->partial_slabs;
    while (*prev && *prev != slab)
      prev = &(*prev)->next;
    if (*prev == slab)
      *prev = slab->next;
    slab->next = cache->empty_slabs;
    cache->empty_slabs = slab;
  }
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);
  if (size == 0) {
    kfree(ptr);
    return NULL;
  }

  slab_t *slab = slab_find(ptr);
  size_t old_size = slab->object_size;

  kmem_cache_t *old_cache = cache_for_size(old_size);
  kmem_cache_t *new_cache = cache_for_size(size);
  if (old_cache && old_cache == new_cache)
    return ptr;

  void *new_ptr = kmalloc(size);
  if (!new_ptr)
    return NULL;

  size_t copy_size = (old_size < size) ? old_size : size;
  memcpy(new_ptr, ptr, copy_size);
  kfree(ptr);
  return new_ptr;
}

void kmalloc_stats_get(kmalloc_stats_t *out) { *out = stats; }

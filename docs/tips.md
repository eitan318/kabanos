int myfs_lookup(MyfsSuperBlock *sb, MyfsInode *dir_inode, const char *name,
                uint32_t *found_ino) {
  if (!dir_inode || !S_ISDIR(dir_inode->mode))
    return -1;
  if (dir_inode->size == 0)
    return -1;

  int entries_count = dir_inode->size / sizeof(MyfsDirEntry);
  MyfsDirEntry *entries = kmalloc(dir_inode->size);
  if (!entries)
    return -1;

  if (myfs_node_read(sb, dir_inode, 0, entries, dir_inode->size) < 0) {
    kfree(entries);
    return -1;
  }

  int idx = myfs_entry_idx_find(entries, entries_count, name);
  if (idx == -1) {
    kfree(entries);
    return -1;
  }

  *found_ino = entries[idx].inode_num;
  kfree(entries);
  return 0;
}




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
    // Remove from full list
    slab_t **prev = &cache->full_slabs;
    while (*prev && *prev != slab) { // here err
      prev = &(*prev)->next;
    }
    if (*prev == slab) {
      *prev = slab->next;
    }

    // Add to partial list
    slab->next = cache->partial_slabs;
    cache->partial_slabs = slab;
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







/ $ tcc -nostdlib /usr/lib/crt0.o no.c -lc -lnosys -o no.ohello.c -lc -lnocsys -o no.ohello.o
Segmentation Fault: No VMA at 0x21
gs: 0x23, gs: 0x23, fs: 0x23, fs: 0x23, es: 0x23, es: 0x23, ds: 0x23, ds: 0x23, edi: 0x0, edi: 0x0, esi: 0x0, esi: 0x0, ebp: 0xf1091bc8, ebp: 0xf1091bc8, esp_dummy: 0xf1091b8c, esp_dummy: 0xf1091b8c, ebx: 0xbfffe0b4, ebx: 0xbfffe0b4, edx: 0xe9a0, edx: 0xe9a0, ecx: 0xe0346800, ecx: 0xe0346800, eax: 0x21, eax: 0x21, int_no: 0xe, int_no: 0xe, err_code: 0x0, err_code: 0x0, eip: 0xc010adf0, eip: 0xc010adf0, cs: 0x8, cs: 0x8, eflags: 0x92, eflags: 0x92, esp_user: 0xe03466a3, esp_user: 0xe03466a3, ss_user: 0xe0000b47, ss_user: 0xe0000b47, cr2: 0x21, cr2: 0x21, cr3: 0x65a000, cr3: 0x65a000,

FAULTING_INSTRUCTION_OF_PANIC: 0xc010adf0
STACK_OF_PANIC: 0xc0106139

--- Panic detected! Resolving addresses ---

FAULT @ 0xc010adf0:
/project/kernel/src/mm/kmalloc.c:338

STACK BACKTRACE:
/project/kernel/src/fs/myfs/myfs.c:869

--- End of panic resolution ---



(gdb) x $esp
0x8ec8: 0x0000004f
(gdb)

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





  virtual addresses on every command, and a stale-page-directory bug that turns the first 4MB-boundary crossing into a
  panic.

  The crash: stale kernel PDEs in process page directories

  The math on the fault address gives it away: KERNEL_HEAP_START = KERNEL_BASE + MAX_PHYSICAL_MEMORY = 0xE0000000, and
  the fault is at 0xE0400000 — the heap start plus exactly 4MB, i.e. the first page-table boundary.

  The chain:

  1. Each process page directory is created by hal_vm_arch_clone_mapping (hal_vmm.c:418), which is a one-time memcpy of
  the whole PD — including the kernel's PDEs as they existed at that moment.
  2. When the kernel heap grows past 4MB, heap_page_alloc → hal_vm_map(kernel_arch_vm, ...) → get_or_create_page_table
  allocates a brand-new page table and writes its PDE only into the kernel's PD (hal_vmm.c:130). Nobody tells the
  already-cloned process PDs.
  3. Your fault happened during a syscall, so cr3 was the process's PD (0xa45000). The kernel's memcpy wrote to the
  freshly-allocated heap page at 0xE0400000, the process PD has no PDE for that 4MB slot → page fault (err_code 0x2 =
  supervisor write to non-present — matches).
  4. pf_handle (page_fault_handler.c:33-38) only searches the current process's vmspace for a VMA. Kernel heap
  addresses aren't in any process VMA → "No VMA at 0xe0400000" → panic.

  It never happened below 0xE0400000 because the heap's first page table was created at boot, before any process
  existed, so every clone inherited it.

  The "budget": heap VAs are never reused

  heap_page_alloc (kmalloc.c:45-70) is a pure bump allocator: next_heap_addr only ever increases. kfree unmaps pages,
  but the virtual address is gone forever — nothing ever recycles it. Every command spawns processes and does large
  kmallocs (ELF buffers etc.), so each command permanently consumes heap VA even though the memory is freed correctly.
  ~12 ls runs ≈ 4MB means each command burns ~340KB of address space. That's your dwindling budget, and crossing 4MB
  detonates bug #1.

  Bonus bugs I hit while reading (not today's crash, but real)

  - kfree of a large (>2048B) allocation frees one page only — it has no record of how many pages the allocation had
  (kmalloc.c:299-301), so multi-page allocations leak physical frames. Related TODO at kmalloc.c:191. Also,
  slab->object_size read from a large-allocation page is garbage — the "cache miss" detection works by luck.
  - page_fault_handler.c:21-26: the pf_errors enum uses 1 >> n instead of 1 << n, so PF_ERR_WRITE, PF_ERR_USER, etc.
  are all 0 — every regs->error & PF_ERR_WRITE check is dead code. PF_ERR_PRESENT works only because 1 >> 0 == 1.

  Fix directions — worth deciding deliberately

  For the crash, the two standard designs are:

  - A. Sync-on-fault (Linux-style): in pf_handle, if addr >= KERNEL_BASE, copy the missing PDE from the kernel PD into
  the current PD and return. No wasted memory, but it relies on the fault handler running — if the unmapped thing were
  ever a kernel stack, you'd double-fault instead.
  - B. Preallocate all kernel-space page tables at boot: create all 256 PTs for the kernel's 1GB half once, so kernel
  PDEs never change after boot and every clone is complete forever. Costs 1MB of frames, kills the entire bug class
  deterministically (including the same latent bug for the process-kernel-stack region at 0xF1..., which your ebp shows
  is in play).

  I'd lean B for robustness and simplicity, with the VA-reuse problem in kmalloc as a separate follow-up (that one
  needs a real design: a free list of VA ranges, and large-allocation size tracking so kfree works).

  Before I write anything — which approach would you take, and do you see a reason the 1MB prealloc cost matters on
  your target? Also, since this touches your allocator design: how were you planning for kfree to learn a large
  allocation's page count? Worth having your own answer to that before I propose one.

✻ Cogitated for 2m 45s
                                                                                                      ● high · /effort
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
❯ k

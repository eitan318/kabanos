// sys_sbrk.c
#include "mm/memdefs.h"
#include "mm/vmspace.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "utils/math.h"

// Returns old brk on success, -1 on failure (matches POSIX sbrk semantics)
uintptr_t sys_sbrk(intptr_t increment) {
  process_t *proc = dispatch_get_current()->process;
  uint32_t old_brk = proc->brk;

  if (increment == 0)
    return old_brk; // just querying current brk

  uint32_t new_brk = old_brk + increment;

  // Align new brk up to page boundary for actual mapping
  uint32_t old_brk_aligned = align_up(old_brk, PAGE_SIZE);
  uint32_t new_brk_aligned = align_up(new_brk, PAGE_SIZE);

  // Only call into vmspace if we actually need new pages
  if (new_brk_aligned > old_brk_aligned) {
    if (!vmspace_extend_heap(proc->vmspace, old_brk_aligned, new_brk_aligned))
      return (uintptr_t)-1;
  }

  proc->brk = new_brk;
  return old_brk; // sbrk returns the OLD break
}

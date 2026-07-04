/**
 * @file sys_sbrk.c
 * @brief sbrk syscall: grows the process heap.
 */
#include "mm/vmspace.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "utils/math.h"

/**
 * @brief Resizes the program break (heap).
 * @param increment Bytes to add (positive) or remove (negative).
 * @return Previous break address on success, or (uintptr_t)-1 on error.
 */
uintptr_t sys_sbrk(intptr_t increment) {
  process_t *proc = dispatch_get_current()->process;
  uint32_t old_brk = proc->brk;

  if (increment == 0)
    return old_brk;

  uint32_t new_brk = old_brk + increment;
  uint32_t old_brk_aligned = align_up(old_brk, PAGE_SIZE);
  uint32_t new_brk_aligned = align_up(new_brk, PAGE_SIZE);

  if (new_brk_aligned > old_brk_aligned) {
    if (!vmspace_extend_heap(proc->vmspace, old_brk_aligned, new_brk_aligned))
      return (uintptr_t)-1;
  }

  proc->brk = new_brk;
  return old_brk;
}

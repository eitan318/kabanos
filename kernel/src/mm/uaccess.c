#include "mm/uaccess.h"
#include "hal.h"
#include "klib/errno.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/thread.h"

int uaccess_copy_to_user(void *user_dst, const void *kernel_src, size_t size) {
  thread_t *curr_thread = dispatch_get_current();
  process_t *curr_proc = curr_thread->process;

  return vmspace_copy_to(curr_proc->vmspace->arch, (vaddr_t)user_dst,
                         (vaddr_t)kernel_src, size);
}

int uaccess_copy_from_user(void *kernel_dst, const void *user_src,
                           size_t size) {
  thread_t *curr_thread = dispatch_get_current();
  process_t *curr_proc = curr_thread->process;

  return vmspace_copy_from(curr_proc->vmspace->arch, (vaddr_t)kernel_dst,
                           (vaddr_t)user_src, size);
}

#include "proc/proc.h"
#include "klib/string.h"
#include "mm/kmalloc.h"
#include "mm/vmspace.h"
#include "sched/thread.h"

static uint32_t next_pid = 1;
static uint32_t alloc_pid() { return next_pid++; }

process_t *process_create(void) {
  process_t *p = kmalloc(sizeof(*p));
  memset(p, 0, sizeof(*p));

  p->pid = alloc_pid();

  return p;
}

void process_destroy(process_t *proc) {
  if (proc->main_thread) {
    proc->main_thread->state = THREAD_DEAD;
  }
  vmspace_destroy(proc->vmspace);
  kfree(proc);
}

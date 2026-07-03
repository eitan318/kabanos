#include "proc/proc.h"
#include "fs/vfs_internal.h"
#include "mm/kmalloc.h"
#include "mm/vmspace.h"
#include "net/net_syscalls.h"
#include "sched/thread.h"

static uint32_t next_pid = 1;
static uint32_t alloc_pid() { return next_pid++; }

process_t *process_create(void) {
  process_t *p = kmalloc(sizeof(*p));
  memset(p, 0, sizeof(*p));

  p->cwd = vfs_lookup_path("/", NULL, true);
  p->pid = alloc_pid();
  return p;
}

void process_destroy(process_t *proc) {
  if (proc->main_thread) {
    proc->main_thread->state = THREAD_STATE_DEAD;
  }
  sys_net_close_all();
  vmspace_destroy(proc->vmspace);
  kfree(proc);
}

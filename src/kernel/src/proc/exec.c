#include "proc/exec.h"
#include "arch/types.h"
#include "elf.h"
#include "fat.h"
#include "hal.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/va_allocation.h"
#include "mm/vmspace.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "stdio.h"

int exec_load_elf(vmspace_t *vm, const char *path, uintptr_t *entry) {
  void *data;
  size_t size;

  if (fat_read_file(path, &data, &size) < 0) {
    return -1;
  }
  int r = elf_load(vm->arch, data, size, entry);
  kfree(data);
  return r;
}

// Return top of stack
uintptr_t alloc_user_stack(vmspace_t *vm) {
  if (!va_alloc_region(vm->arch, USER_STACK_BOTTOM + 1, USER_STACK_SIZE,
                       PAGE_USER | PAGE_READWRITE)) {
    kdebugf("user stack creation failed");
    return -1;
  }

  return (uintptr_t)(USER_STACK_BOTTOM + USER_STACK_SIZE);
}

void free_user_stack(vmspace_t *vm) {
  va_free_region(vm->arch, USER_STACK_BOTTOM + 1, USER_STACK_SIZE);
}

long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
  thread_t *current = dispatch_get_current();
  process_t *proc = current->process;

  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, pathname, &entry) < 0) {
    kdebugf("elf load failed");
    return -1;
  }

  free_user_stack(proc->vmspace);
  uintptr_t user_stack = alloc_user_stack(proc->vmspace);

  hal_thread_init(current, entry, user_stack);
  return 0;
}

int process_spawn(const char *path, enum thread_priority p) {
  process_t *proc = process_create();
  proc->vmspace = vmspace_create();
  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, path, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }

  uintptr_t user_stack = alloc_user_stack(proc->vmspace);
  thread_t *t = thread_create_user(proc, entry, user_stack, p);

  if (proc && !proc->main_thread) {
    proc->main_thread = t;
  }

  sched_enqueue(t);

  return 0;
}

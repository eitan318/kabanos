#include "proc/exec.h"
#include "fat/fat.h"
#include "loader/elf.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmspace.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

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
uintptr_t setup_user_stack(vmspace_t *vm) {
  if (!va_alloc_region(vm->arch, USER_STACK_BOTTOM + 1, USER_STACK_SIZE,
                       PAGE_USER | PAGE_READWRITE)) {
    return -1;
  }
  return (uintptr_t)(USER_STACK_BOTTOM + USER_STACK_SIZE);
}

int process_exec_noreturn(const char *path, enum thread_priority p) {
  thread_t *current = dispatch_get_current();
  process_t *proc = current->process;

  // 1. Validate the new ELF before we destroy the current address space
  uintptr_t entry;
  // Note: You'll need a way to "clear" or "reset" the vmspace
  // instead of just creating a new one.
  if (exec_load_elf(proc->vmspace, path, &entry) < 0) {
    return -1; // Fail before destroying the process
  }

  // 2. Reset User Stack
  uintptr_t user_stack = setup_user_stack(proc->vmspace);

  // 3. Update the current thread's instruction pointer and stack pointer
  // This usually requires a helper to modify the saved register state
  // so when we return to user-space, we land at 'entry'.
  hal_thread_set_userspace_state(current, entry, user_stack);

  return 0; // The syscall return logic will jump to the NEW entry point
}

int process_spawn(const char *path, enum thread_priority p) {
  process_t *proc = process_create();
  proc->vmspace = vmspace_create();
  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, path, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }

  uintptr_t user_stack = setup_user_stack(proc->vmspace);
  thread_t *t = thread_create_user(proc, entry, user_stack, p);

  if (proc && !proc->main_thread) {
    proc->main_thread = t;
  }

  sched_enqueue(t);

  return 0;
}

#include "proc/exec.h"
#include "fat/fat.h"
#include "loader/elf.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmspace.h"
#include "proc/proc.h"
#include "sched/thread.h"

int exec_load_elf(vmspace_t *vm, const char *path, uintptr_t *entry) {
  void *data;
  size_t size;

  fat_read_file(path, &data, &size);
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

int process_exec(const char *path) {
  process_t *proc = process_create();
  vmspace_t *vm = vmspace_create();

  proc->vmspace = vm;

  uintptr_t entry;
  if (exec_load_elf(vm, path, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }

  uintptr_t user_stack = setup_user_stack(vm);
  thread_t *t = thread_create_user(proc, entry, user_stack);
  return 0;
}

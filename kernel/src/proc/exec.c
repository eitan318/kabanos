#include "proc/exec.h"
#include "elf32.h"
#include "fs/vfs.h"
#include "hal.h"
#include "klib/errno.h"
#include "klib/stdio.h"
#include "ksys/fcntl.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/va_allocation.h"
#include "mm/vmspace.h"
#include "proc/exec_table.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"

#define MAX_ARGC 1024

int exec_load_elf(vmspace_t *vm, const char *path, uintptr_t *entry) {

  int fd = vfs_open(path, O_RDONLY);
  if (fd < 0) {
    return fd;
  }

  fstat_t st;
  if (vfs_fstat(fd, &st) < 0) {
    vfs_close(fd);
    return -EIO;
  }

  void *data = kmalloc(st.size);
  if (!data) {
    vfs_close(fd);
    return -ENOMEM;
  }

  ssize_t nread = vfs_read(fd, data, st.size);
  vfs_close(fd);

  if (nread < 0 || (size_t)nread != (size_t)st.size) {
    kfree(data);
    return -EIO;
  }

  uintptr_t load_base = 0;
  int r = elf32_load(vm->arch, data, st.size, entry, &load_base);
  if (r == 0) {
    exec_table_add(path, load_base);

  } else {
    kdebugf("exec_load_elf: elf_load failed: %d\n", r);
  }

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

int count_args(char *const argv[]) {
  int argc = 0;
  if (argv == NULL)
    return 0;

  while (argv[argc] != NULL) {
    argc++;
    // Safety: Prevent infinite loops if user passes garbage
    if (argc > MAX_ARGC)
      return -1;
  }
  return argc;
}

/**
 * @brief Prepares the user-mode stack with command-line arguments (argc, argv).
 * * This function performs a "double-copy" or "remote-write" to initialize a
 * new process's stack. It copies actual string data, the array of pointers
 * (argv), and the initial stack frame required by the System V i386 ABI.
 * * @param[in]  dst_vm    Pointer to the destination virtual memory space.
 * @param[in]  stack_top The initial top of the allocated user stack (highest
 * address).
 * @param[in]  argc      The number of arguments in the argv array.
 * @param[in]  argv      An array of strings in the current address space to be
 * copied.
 * * @return The final value of the user stack pointer (ESP) to be set in the
 * thread's CPU context.
 * * @note This function ensures 4-byte alignment for strings and pointers.
 * It also null-terminates the argv array on the user stack as per
 * standard C conventions.
 */
static uintptr_t setup_user_stack_args(vmspace_t *dst_vm, uintptr_t stack_top,
                                       int argc, char *const argv[]) {
  uintptr_t user_sp = stack_top;

  uintptr_t user_argv_ptrs[argc + 1]; // +1 for null-terminat

  // Copy args
  for (int i = argc - 1; i >= 0; i--) {
    uintptr_t arg_size = strlen(argv[i]);

    // push argv[i]
    user_sp -= arg_size;
    hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&argc, sizeof(int));

    user_argv_ptrs[i] = user_sp;
  }

  user_argv_ptrs[argc] = '\0';

  uint32_t user_argv_ptrs_size = sizeof(user_argv_ptrs) * (argc + 1);

  // Push argv
  user_sp -= user_argv_ptrs_size;
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&user_sp,
                         user_argv_ptrs_size);

  // Structure: [argc] [argv_ptr] [envp_ptr]
  uintptr_t argv_ptr_for_stack = user_sp;

  // Push envp
  user_sp -= sizeof(uintptr_t); // envp (NULL for now)
  uintptr_t null_env = 0;
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&null_env,
                         sizeof(uintptr_t));

  // Push argv
  user_sp -= sizeof(uintptr_t);
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&argv_ptr_for_stack,
                         sizeof(uintptr_t));

  // Push argc
  user_sp -= sizeof(argc);
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&argc, sizeof(int));

  return user_sp;
}

long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
  thread_t *current = dispatch_get_current();
  process_t *proc = current->process;

  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, pathname, &entry) < 0)
    return -ENOENT;

  free_user_stack(proc->vmspace);
  uintptr_t stack_top = alloc_user_stack(proc->vmspace);

  // Count args
  // int argc = 0;
  // while (argv[argc])
  // argc++;

  uintptr_t user_sp;
  // Setup strings and pointers in the new vmspace
  // uintptr_t user_sp =
  //    setup_user_stack_args(proc->vmspace, stack_top, argc, argv);

  user_sp = stack_top;
  hal_thread_init(current, entry, user_sp);
  return 0;
}

int process_spawn(const char *path, int argc, char *const argv[],
                  enum thread_priority p) {
  process_t *proc = process_create();
  proc->vmspace = vmspace_create();

  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, path, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }

  uintptr_t stack_top = alloc_user_stack(proc->vmspace);

  uintptr_t user_sp;
  // Setup the stack with arguments
  // uintptr_t user_sp =
  //   setup_user_stack_args(proc->vmspace, stack_top, argc, argv);
  //
  user_sp = stack_top;
  thread_t *t = thread_create_user(proc, entry, user_sp, p);
  proc->main_thread = t;

  sched_enqueue(t);
  return 0;
}

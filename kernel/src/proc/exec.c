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
  paddr_t path_phys = hal_vm_virt_to_phys(vm->arch, (vaddr_t)path);

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

  uintptr_t text_base = 0;
  int r = elf32_load(vm->arch, data, st.size, entry, &text_base);

  if (r == 0) {
    exec_table_add(path, text_base);

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
                                       int argc, char *const argv[],
                                       char *const envp[]) {
  uintptr_t user_sp = stack_top;

  // Use a fixed limit or kmalloc for the pointers to avoid kernel stack
  // overflow
  uintptr_t user_argv_ptrs[argc + 1];

  // 1. Copy the actual strings first (Data area)
  for (int i = argc - 1; i >= 0; i--) {
    size_t len = strlen(argv[i]) + 1; // +1 for null terminator
    user_sp -= len;

    // Align to 4 bytes (System V ABI requirement)
    user_sp &= ~0x3;

    // Copy the ACTUAL string data from current VM to destination VM
    hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)argv[i], len);

    // Save the address WHERE WE PUT IT in the new VM
    user_argv_ptrs[i] = user_sp;
  }
  user_argv_ptrs[argc] = 0; // NULL terminator for argv array

  // Copy the array of pointers (argv array)
  size_t ptr_array_size = sizeof(uintptr_t) * (argc + 1);
  user_sp -= ptr_array_size;
  user_sp &= ~0x3;
  uintptr_t argv_array_base = user_sp;

  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)user_argv_ptrs,
                         ptr_array_size);

  // 3. Setup the final stack frame: [argc] [argv_ptr] [envp_ptr] [exit_addr]
  // Note: Some ABIs expect a dummy return address at the very top.

  // Push envp (NULL)
  uintptr_t null_env = 0;
  user_sp -= sizeof(uintptr_t);
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&null_env,
                         sizeof(uintptr_t));

  // Push argv pointer (pointer to the array we just created)
  user_sp -= sizeof(uintptr_t);
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&argv_array_base,
                         sizeof(uintptr_t));

  // Push argc
  user_sp -= sizeof(int);
  hal_vm_copy_to_vmspace(dst_vm->arch, user_sp, (vaddr_t)&argc, sizeof(int));

  return user_sp;
}

typedef struct {
  char **kargv;
  char **kenvp;
  int argc;
} captured_args_t;

static captured_args_t capture_args(char *const argv[], char *const envp[]) {
  captured_args_t args = {NULL, 0};
  while (argv[args.argc])
    args.argc++;

  args.kargv = kmalloc(sizeof(char *) * (args.argc + 1));

  for (int i = 0; i < args.argc; i++) {
    // strdup captures the actual string data into kernel heap
    args.kargv[i] = strdup(argv[i]);
  }
  args.kargv[args.argc] = NULL;

  // Not implemented yet
  args.kenvp = NULL;

  return args;
}

long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
  // Capture EVERYTHING to kernel heap while old_vm is still active
  char *kpath = strdup(pathname);
  captured_args_t captured = capture_args(argv, envp);

  // Prepare the new vm
  vmspace_t *new_vm = vmspace_create();
  uintptr_t entry = 0;
  exec_load_elf(new_vm, kpath, &entry);
  uintptr_t stack_top = alloc_user_stack(new_vm);

  // Perform the switch
  thread_t *current = dispatch_get_current();
  vmspace_t *old_vm = current->process->vmspace;

  current->process->vmspace = new_vm;
  vmspace_switch(new_vm);

  uintptr_t user_sp = setup_user_stack_args(new_vm, stack_top, captured.argc,
                                            captured.kargv, captured.kenvp);

  // Cleanup
  vmspace_destroy(old_vm);
  for (int i = 0; i < captured.argc; i++) {
    kfree(captured.kargv[i]);
  }
  kfree(captured.kargv);
  kfree(kpath);

  hal_thread_init(current, entry, user_sp);
  return 0;
}

int process_spawn(const char *path, int argc, char *const argv[],
                  char *const envp[], enum thread_priority p) {

  process_t *proc = process_create();
  proc->vmspace = vmspace_create();

  uintptr_t entry;
  if (exec_load_elf(proc->vmspace, path, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }
  uintptr_t stack_top = alloc_user_stack(proc->vmspace);

  uintptr_t user_sp =
      setup_user_stack_args(proc->vmspace, stack_top, argc, argv, envp);

  thread_t *t = thread_create_user(proc, entry, user_sp, p);
  proc->main_thread = t;

  sched_enqueue(t);
  return 0;
}

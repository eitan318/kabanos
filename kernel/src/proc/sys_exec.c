/**
 * @file exec.c
 * @brief Implementation of process execution and ELF loading.
 */

#include "proc/sys_exec.h"
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
#include "utils/math.h"

#define MAX_ARGC 1024

/**
 * @brief Loads an ELF executable into a virtual address space.
 * * Opens the file, reads it into kernel memory, parses the ELF headers,
 * and maps the segments into the target VM.
 * * @param vm The destination virtual memory space.
 * @param path Filesystem path to the ELF file.
 * @param entry [out] Pointer to store the program entry point (EIP).
 * @return 0 on success, negative errno on failure.
 */
static int exec_load_elf(vmspace_t *vm, const char *path, vnode_t *cwd,
                         uintptr_t *entry) {
  paddr_t path_phys = hal_vm_virt_to_phys(vm->arch, (vaddr_t)path);

  int fd = vfs_open(path, cwd, O_RDONLY, -1);
  if (fd < 0)
    return fd;

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
  int r = elf32_load(vm, data, st.size, entry, &text_base);

  if (r == 0) {
    exec_table_add(path, text_base);
  } else {
    kdebugf("exec_load_elf: elf_load failed: %d\n", r);
  }

  kfree(data);
  return r;
}

/**
 * @brief Initializes the user stack with System V i386 ABI arguments.
 * * Copies argument strings and the argv pointer array into the target VM.
 * * @param dst_vm Target virtual memory space.
 * @param stack_top The high address of the allocated stack.
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 * @param envp Array of environment strings.
 * @return The adjusted stack pointer (ESP) for the entry point.
 */
static uintptr_t setup_user_stack_args(vmspace_t *dst_vm, uintptr_t stack_top,
                                       int argc, char *const argv[],
                                       char *const envp[]) {
  uintptr_t user_sp = stack_top;
  uintptr_t user_argv_ptrs[argc + 1];

  // 1. Copy strings to stack
  for (int i = argc - 1; i >= 0; i--) {
    size_t len = strlen(argv[i]) + 1;
    user_sp -= len;
    user_sp &= ~0x3; // 4-byte alignment
    vmspace_copy_to(dst_vm->arch, user_sp, (vaddr_t)argv[i], len);
    user_argv_ptrs[i] = user_sp;
  }
  user_argv_ptrs[argc] = 0;

  // 2. Copy argv pointer array to stack
  size_t ptr_array_size = sizeof(uintptr_t) * (argc + 1);
  user_sp -= ptr_array_size;
  user_sp &= ~0x3;
  uintptr_t argv_array_base = user_sp;
  vmspace_copy_to(dst_vm->arch, user_sp, (vaddr_t)user_argv_ptrs,
                  ptr_array_size);

  // 3. Construct ABI stack frame
  uintptr_t null_env = 0;
  user_sp -= sizeof(uintptr_t); // envp
  vmspace_copy_to(dst_vm->arch, user_sp, (vaddr_t)&null_env, sizeof(uintptr_t));

  user_sp -= sizeof(uintptr_t); // argv pointer
  vmspace_copy_to(dst_vm->arch, user_sp, (vaddr_t)&argv_array_base,
                  sizeof(uintptr_t));

  user_sp -= sizeof(int); // argc
  vmspace_copy_to(dst_vm->arch, user_sp, (vaddr_t)&argc, sizeof(int));

  return user_sp;
}

typedef struct {
  char **kargv;
  char **kenvp;
  int argc;
} captured_args_t;

/**
 * @brief Copies user-space arguments into kernel-space buffers.
 * * This is necessary to preserve arguments before switching address spaces.
 * * @param argv User-space argument array.
 * @param envp User-space environment array.
 * @return A structure containing the kernel-side copies.
 */
static captured_args_t capture_args(char *const argv[], char *const envp[]) {
  captured_args_t args = {NULL, NULL, 0};
  while (argv[args.argc])
    args.argc++;

  args.kargv = kmalloc(sizeof(char *) * (args.argc + 1));
  for (int i = 0; i < args.argc; i++) {
    args.kargv[i] = strdup(argv[i]);
  }
  args.kargv[args.argc] = NULL;
  args.kenvp = NULL; // TODO: Implement envp capture

  return args;
}

long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
  char *kpath = strdup(pathname);
  captured_args_t captured = capture_args(argv, envp);
  thread_t *current = dispatch_get_current();

  vmspace_t *new_vm = vmspace_create();
  uintptr_t entry = 0;
  if (exec_load_elf(new_vm, kpath, current->process->cwd, &entry) < 0) {
    kdebugf("Couldnt load: %s");
    vmspace_destroy(new_vm);
    return -ENOENT;
  }

  uintptr_t stack_top = USER_STACK_BOTTOM + USER_STACK_SIZE;
  vmspace_map_stack(new_vm, stack_top, USER_STACK_SIZE);

  vmspace_map_heap(new_vm, USER_HEAP_START, USER_HEAP_INITIAL);
  current->process->heap_start = USER_HEAP_START;
  current->process->brk = USER_HEAP_START + USER_HEAP_INITIAL;

  vmspace_t *old_vm = current->process->vmspace;

  current->process->vmspace = new_vm;
  vmspace_switch(new_vm);

  uintptr_t user_sp = setup_user_stack_args(new_vm, stack_top, captured.argc,
                                            captured.kargv, captured.kenvp);

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
  if (exec_load_elf(proc->vmspace, path, proc->cwd, &entry) < 0) {
    process_destroy(proc);
    return -1;
  }

  uintptr_t stack_top = USER_STACK_BOTTOM + USER_STACK_SIZE;
  vmspace_map_stack(proc->vmspace, stack_top, USER_STACK_SIZE);

  uintptr_t user_sp =
      setup_user_stack_args(proc->vmspace, stack_top, argc, argv, envp);

  thread_t *t = thread_create_user(proc, entry, user_sp, p);
  proc->main_thread = t;

  sched_enqueue(t);
  return 0;
}

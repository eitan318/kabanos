#include "syscall.h"
#include "drivers/console/sys_console.h"
#include "fs/sys_fs.h"
#include "klib/time.h"
#include "mm/sys_sbrk.h"
#include "net/net_syscalls.h"
#include "proc/proc.h"
#include "proc/sys_exec.h"
#include "proc/sys_proc.h"
#include "proc/sys_wait.h"
#include "sched/sleep.h"
#include "sched/sys_yield.h"
#include "sys_cwd.h"
#include "sys_ioctl.h"
#include "sys_time.h"
#include <fs/vfs.h>
#include <stdint.h>

typedef enum {
  SYSCALL_NUMBER_INITIAL = 1,

  // --- File System ---
  SYSCALL_NUMBER_SYS_WRITE,
  SYSCALL_NUMBER_SYS_READ,
  SYSCALL_NUMBER_SYS_OPEN,
  SYSCALL_NUMBER_SYS_CLOSE,
  SYSCALL_NUMBER_SYS_LSEEK,
  SYSCALL_NUMBER_SYS_FSTAT,
  SYSCALL_NUMBER_SYS_LSTAT,
  SYSCALL_NUMBER_SYS_STAT,
  SYSCALL_NUMBER_SYS_GETDENTS,
  SYSCALL_NUMBER_SYS_CREATE,
  SYSCALL_NUMBER_SYS_UNLINK,
  SYSCALL_NUMBER_SYS_RENAME,
  SYSCALL_NUMBER_SYS_MKDIR,
  SYSCALL_NUMBER_SYS_RMDIR,
  SYSCALL_NUMBER_SYS_SYMLINK,
  SYSCALL_NUMBER_SYS_READLINK,
  SYSCALL_NUMBER_SYS_LINK,
  SYSCALL_NUMBER_SYS_MOUNT,
  SYSCALL_NUMBER_SYS_UMOUNT,
  SYSCALL_NUMBER_SYS_GETCWD,
  SYSCALL_NUMBER_SYS_CHDIR,

  // --- Process & Lifecycle ---
  SYSCALL_NUMBER_SYS_FORK,
  SYSCALL_NUMBER_SYS_EXECVE,
  SYSCALL_NUMBER_SYS_EXIT,
  SYSCALL_NUMBER_SYS_WAITPID,
  SYSCALL_NUMBER_SYS_GETPID,

  // --- Scheduling & Time ---
  SYSCALL_NUMBER_SYS_YIELD,
  SYSCALL_NUMBER_SYS_SLEEP,
  SYSCALL_NUMBER_SYS_NANOSLEEP,
  SYSCALL_NUMBER_SYS_GETTIMEOFDAY,
  SYSCALL_NUMBER_SYS_TIMES,

  // --- Memory Management ---
  SYSCALL_NUMBER_SYS_SBRK,
  SYSCALL_NUMBER_SYS_MMAP,
  SYSCALL_NUMBER_SYS_MUNMAP,
  SYSCALL_NUMBER_SYS_MPROTECT,

  // --- Signals & IPC ---
  SYSCALL_NUMBER_SYS_PIPE,
  SYSCALL_NUMBER_SYS_SIGACTION,
  SYSCALL_NUMBER_SYS_SIGPROCMASK,
  SYSCALL_NUMBER_SYS_KILL,

  // --- Networking (Sockets) ---
  SYSCALL_NUMBER_SYS_SOCKET,
  SYSCALL_NUMBER_SYS_BIND,
  SYSCALL_NUMBER_SYS_SENDTO,
  SYSCALL_NUMBER_SYS_RECVFROM,
  SYSCALL_NUMBER_SYS_ARP_RESOLVE,

  // --- Console ---
  SYSCALL_NUMBER_SYS_CLEAR,

  // --- IO ---
  SYSCALL_NUMBER_SYS_IOCTL,
} SYSCALL_NUMBER;

long syscall_dispatch(syscall_info_t f) {
  switch (f.num) {
  /* --- File & Device I/O --- */
  case SYSCALL_NUMBER_SYS_OPEN:
    return sys_open((const char *)f.args[0], (int)f.args[1]);
  case SYSCALL_NUMBER_SYS_CLOSE:
    if ((int)f.args[0] >= 64) // socket fd
      return sys_net_close((int)f.args[0]);
    return sys_close((int)f.args[0]);
  case SYSCALL_NUMBER_SYS_READ:
    return sys_read((int)f.args[0], (char *)f.args[1], (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_WRITE:
    return sys_write((int)f.args[0], (const char *)f.args[1],
                     (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_LSEEK:
    return sys_lseek((int)f.args[0], (off_t)f.args[1], (int)f.args[2]);
  case SYSCALL_NUMBER_SYS_FSTAT:
  case SYSCALL_NUMBER_SYS_STAT:
    return sys_stat((int)f.args[0], (fstat_t *)f.args[1]);
  case SYSCALL_NUMBER_SYS_GETDENTS:
    return sys_getdents(f.args[0], (vdir_entry_t *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_CREATE:
    return sys_create((const char *)f.args[0]);

  /* --- Directory & Path Ops --- */
  case SYSCALL_NUMBER_SYS_MKDIR:
    return sys_mkdir((const char *)f.args[0], (mode_t)f.args[1]);
  case SYSCALL_NUMBER_SYS_RMDIR:
    return sys_rmdir((const char *)f.args[0]);
  case SYSCALL_NUMBER_SYS_UNLINK:
    return sys_unlink((const char *)f.args[0]);
  case SYSCALL_NUMBER_SYS_RENAME:
    return sys_rename((const char *)f.args[0], (const char *)f.args[1]);
  case SYSCALL_NUMBER_SYS_SYMLINK:
    return sys_symlink((const char *)f.args[0], (const char *)f.args[1]);
  case SYSCALL_NUMBER_SYS_READLINK:
    return sys_readlink((const char *)f.args[0], (char *)f.args[1],
                        (size_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_CHDIR:
    return sys_chdir((const char *)f.args[0]);
  case SYSCALL_NUMBER_SYS_GETCWD:
    return sys_getcwd((char *)f.args[0], (size_t)f.args[1]);
  case SYSCALL_NUMBER_SYS_LINK:
    return -ENOSYS; // Unimplemented

  /* --- Mount Management --- */
  case SYSCALL_NUMBER_SYS_MOUNT:
    return sys_mount((const char *)f.args[0], (const char *)f.args[1],
                     (const char *)f.args[2], (unsigned long)f.args[3],
                     (void *)f.args[4]);
  case SYSCALL_NUMBER_SYS_UMOUNT:
    return sys_umount((const char *)f.args[0]);

  /* --- Process Management --- */
  case SYSCALL_NUMBER_SYS_FORK:
    return sys_fork();
  case SYSCALL_NUMBER_SYS_EXECVE:
    return sys_execve((const char *)f.args[0], (char *const *)f.args[1],
                      (char *const *)f.args[2]);
  case SYSCALL_NUMBER_SYS_EXIT:
    sys_exit((int)f.args[0]);
    return 0;
  case SYSCALL_NUMBER_SYS_WAITPID:
    return sys_waitpid((pid_t)f.args[0], (int *)f.args[1], (int)f.args[2]);
  case SYSCALL_NUMBER_SYS_GETPID:
    return sys_getpid();

    /* --- Scheduling & Time --- */
  case SYSCALL_NUMBER_SYS_YIELD:
    sys_yield();
    return 0;
  case SYSCALL_NUMBER_SYS_SLEEP:
    sys_sleep((unsigned int)f.args[0]);
    return 0;
  case SYSCALL_NUMBER_SYS_NANOSLEEP:
    return sys_nanosleep((const timespec_t *)f.args[0],
                         (timespec_t *)f.args[1]);
  case SYSCALL_NUMBER_SYS_GETTIMEOFDAY:
    return sys_gettimeofday((timespec_t *)f.args[0], (void *)f.args[1]);
  case SYSCALL_NUMBER_SYS_TIMES: {
    return (long)timer_get_ticks();
  }

  /* --- Memory Management --- */
  case SYSCALL_NUMBER_SYS_SBRK:
    return sys_sbrk((intptr_t)f.args[0]);
  case SYSCALL_NUMBER_SYS_MMAP:
  case SYSCALL_NUMBER_SYS_MUNMAP:
    return -ENOSYS; // Unimplemented

  /* --- Signals & IPC --- */
  case SYSCALL_NUMBER_SYS_KILL:
  case SYSCALL_NUMBER_SYS_PIPE:
  case SYSCALL_NUMBER_SYS_SIGACTION:
    return -ENOSYS; // Unimplemented

  case SYSCALL_NUMBER_SYS_SOCKET:
    return sys_socket((int)f.args[0], (int)f.args[1], (int)f.args[2]);
  case SYSCALL_NUMBER_SYS_BIND:
    return sys_bind((int)f.args[0], (struct sockaddr *)f.args[1],
                    (uint32_t)f.args[2]);
  case SYSCALL_NUMBER_SYS_SENDTO:
    return sys_sendto((int)f.args[0], (void *)f.args[1], (size_t)f.args[2],
                      (int)f.args[3], (struct sockaddr *)f.args[4],
                      (uint32_t)f.args[5]);
  case SYSCALL_NUMBER_SYS_RECVFROM:
    return sys_recvfrom((int)f.args[0], (void *)f.args[1], (size_t)f.args[2],
                        (int)f.args[3], (struct sockaddr *)f.args[4],
                        (uint32_t *)f.args[5]);

    return sys_recvfrom((int)f.args[0], (void *)f.args[1], (size_t)f.args[2],
                        (int)f.args[3], (struct sockaddr *)f.args[4],
                        (uint32_t *)f.args[5]);
  case SYSCALL_NUMBER_SYS_ARP_RESOLVE:
    return sys_arp_resolve((uint8_t *)f.args[0], (uint8_t *)f.args[1]);

  case SYSCALL_NUMBER_SYS_CLEAR:
    return sys_clear();
  case SYSCALL_NUMBER_SYS_IOCTL:
    return sys_ioctl((int)f.args[0], (unsigned long)f.args[1],
                     (void *)f.args[2]);

  default:
    return -EINVAL;
  }
}

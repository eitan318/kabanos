#include "syscall.h"
#include "hal.h"
#include "klib/stdlib.h"
#include "mm/kmalloc.h"
#include "syscall.h"

void syscall_handler_entry(trap_frame_t *regs) {
  syscall_info_t info;

  // EAX and EBX are preserved as-is
  info.num = regs->eax;
  info.args[0] = regs->ebx; // a1

  // ECX is our pointer to the User Stack where we pushed a2-a6
  uint32_t *user_args = (uint32_t *)regs->ecx;

  // Pull from the stack in the order they were pushed
  info.args[1] = user_args[0]; // a2
  info.args[2] = user_args[1]; // a3
  info.args[3] = user_args[2]; // a4
  info.args[4] = user_args[3]; // a5
  info.args[5] = user_args[4]; // a6

  info.context = regs;

  long result = syscall_dispatch(info);
  regs->eax = (uint32_t)result;
}

#include "syscall/syscall.h"
#include "hal.h"
#include "memory_management/kmalloc.h"
#include "syscall.h"
#include "types.h"
#include <stdlib.h>

void syscall_handler_entry(arch_regs *regs) {
  // 2. Map registers to a generic syscall info structure
  // Standard x86 syscall convention: eax=num, ebx=arg0, ecx=arg1, edx=arg2,
  // esi=arg3, edi=arg4, ebp=arg5
  syscall_info_t info;
  info.num = regs->eax;
  info.args[0] = regs->ebx;
  info.args[1] = regs->ecx;
  info.args[2] = regs->edx;
  info.args[3] = regs->esi;
  info.args[4] = regs->edi;
  info.args[5] = regs->ebp;
  info.context = regs;

  // 3. Dispatch to the generic handler
  long result = syscall_dispatch(info);

  // 4. Store return value in EAX slot so user-space gets it
  regs->eax = (uint32_t)result;
}

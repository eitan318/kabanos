#include "arch/i686/regs.h"
#include "hal.h"
#include "syscall/syscall.h"

void hal_syscall_isr_handler(struct regs *r) {
  syscall_frame_t frame = {
      .num = r->eax, .args = {r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp}};

  r->eax = syscall_dispatch(&frame);
}

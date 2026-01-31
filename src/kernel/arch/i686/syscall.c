#include "syscall/syscall.h"
#include "hal.h"
#include "isr.h"

#define SYSCALL_INTERRUPT 0x80

static void syscall_isr_handler(struct arch_regs *r) {
  syscall_frame_t frame = {
      .num = r->eax, .args = {r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp}};

  r->eax = syscall_dispatch(&frame);
}

void i686_syscall_init() {
  isr_handler_register(SYSCALL_INTERRUPT, syscall_isr_handler);
}

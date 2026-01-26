#include "arch/i686/regs.h"
#include "isr.h"
#include "stdio.h"
#include <stddef.h>
#define SYSCALL_INTERRUPT 0x80

typedef enum { SYSCALL_NUMBERS_SYS_WRITE = 1 } SYSCALL_NUMBERS;

void syscall_isr_handler(struct regs *regs) {
  uint32_t syscall_num = regs->eax;
  uint32_t arg1 = regs->ebx;
  uint32_t arg2 = regs->ecx;

  switch (syscall_num) {
  case SYSCALL_NUMBERS_SYS_WRITE:
    char *str = (char *)arg1;
    size_t len = arg2;
    printf("%s", str);
    break;
  default:
    break;
  }
}

void syscall_init() {
  isr_handler_register(SYSCALL_INTERRUPT, syscall_isr_handler);
}

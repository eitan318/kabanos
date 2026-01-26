#include "isr.h"
#include "stdio.h"
#include "arch/i686/regs.h"
#include <stddef.h>
#define SYSCALL_INTERRUPT 0x80

typedef enum {
	SYSCALL_NUMBERS_SYS_WRITE=1
} SYSCALL_NUMBERS;

void syscall_isr_handler(struct regs *regs) {
	debugf("[SYSCALL] num=%d, arg1=%x, arg2=%x\n", 
           regs->eax, regs->ebx, regs->ecx);

	uint32_t syscall_num = regs->eax;
	uint32_t arg1 = regs->ebx;
	uint32_t arg2 = regs->ecx;
	
	switch (syscall_num)
	{
		case SYSCALL_NUMBERS_SYS_WRITE:
			char *str = (char*)arg1;
			size_t len = arg2;
			
			for (size_t i = 0; i < len; i++) {
				debugc(str[i]);
			}
			break;
		default:
			break;
	}
}

void syscall_init() {
  isr_handler_register(SYSCALL_INTERRUPT, syscall_isr_handler);
}

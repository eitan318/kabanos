#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/interrupts.h"
#include "arch/i686/pic.h"
#include "arch/i686/regs.h"
#include "arch/i686/syscall.h"
#include "arch/i686/timer.h"
#include "assert.h"
#include "panic.h"
#include "string.h"
#include <stdint.h>

void hal_interrupts_disable(void) { asm volatile("cli"); }
void hal_interrupts_enable(void) { asm volatile("sti"); }
void hal_halt(void) { asm volatile("hlt"); }
void hal_trap() { __asm__ volatile("int $3"); }

int hal_interrupts_state_get() {
  uint32_t eflags;
  __asm__ volatile("pushf; pop %0" : "=r"(eflags));
  return eflags & 0x200;
}
int hal_regs_interrupt_number(struct regs *regs) {
  i686_regs_t *r = regs;
  return r->interrupt;
}
uintptr_t hal_regs_pc(struct regs *regs) {
  i686_regs_t *r = regs;
  return r->eip;
}

void hal_serial_putc(const char c) { hal_out8(0x3f8, c); }

void hal_arch_init(void) {
  i686_gdt_init();
  i686_idt_init();
  i686_isr_init();
  i686_pic_init();
  i686_timer_init();
  i686_syscall_init();
}

#define i686_MAX_REGS 16
unsigned hal_regs_max_get() { return i686_MAX_REGS; }

bool hal_regs_from_user(const struct regs *regs) {
  i686_regs_t *r = regs;
  return (r->cs & 0x3) != 0;
}

int hal_describe_regs(i686_regs_t *regs, int max_regs, const char **names,
                      uintptr_t *values) {
  if (max_regs != i686_MAX_REGS)
    return -1;
  if (!regs)
    panic_halt("describe_regs(NULL)!");

  static const char *_names[] = {"eax", "ecx", "edx", "ebx",    "esi", "edi",
                                 "eip", "ebp", "esp", "eflags", "cs",  "U-esp",
                                 "cr0", "cr2", "cr3", "cr4"};
  memcpy((uint8_t *)names, (uint8_t *)_names,
         sizeof(const char *) * i686_MAX_REGS);

  values[0] = regs->eax;
  values[1] = regs->ecx;
  values[2] = regs->edx;
  values[3] = regs->ebx;
  values[4] = regs->esi;
  values[5] = regs->edi;
  values[6] = regs->eip;
  values[7] = regs->esp_dummy;
  values[8] = regs->esp_dummy;
  values[9] = regs->eflags;
  values[10] = regs->cs;
  values[11] = regs->esp_user;

  __asm__ volatile("mov %%cr0, %0" : "=r"(values[12]));
  __asm__ volatile("mov %%cr2, %0" : "=r"(values[13]));
  __asm__ volatile("mov %%cr3, %0" : "=r"(values[14]));
  __asm__ volatile("mov %%cr4, %0" : "=r"(values[15]));

  return 16;
}

uintptr_t hal_backtrace(uintptr_t *data, struct regs *regs) {
  i686_regs_t *arch_regs = regs;
  if (*data == 0) {
    if (regs)
      *data = arch_regs->ebp;
    else
      __asm__ volatile("mov %%ebp, %0" : "=r"(*data));
  }

  uintptr_t past_instruction_ptr = *(uintptr_t *)(*data + 4);
  *data = *(uintptr_t *)(*data - sizeof(uint32_t));

  if (*data == 0)
    return 0;
  return past_instruction_ptr;
}

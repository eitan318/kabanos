#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/interrupts.h"
#include "arch/i686/pic.h"
#include "arch/i686/sysenter.h"
#include "arch/i686/timer.h"
#include "arch/i686/types.h"
#include "assert.h"
#include "modules/modules.h"
#include "panic.h"
#include "sched/sched.h"
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
int hal_regs_interrupt_number(struct trap_frame *regs) {
  return regs->interrupt;
}
uintptr_t hal_regs_pc(struct trap_frame *regs) { return regs->eip; }

void hal_serial_putc(const char c) { hal_out8(0x3f8, c); }

int hal_arch_init(module_t *self) {
  i686_gdt_init();
  i686_idt_init();
  i686_isr_init();
  i686_pic_init();
  i686_timer_init(1000 / TIMER_TICK_MS);
  i686_sysenter_init();
  return 0;
}

#define i686_MAX_REGS 21
unsigned hal_regs_max_get() { return i686_MAX_REGS; }

bool hal_regs_from_user(const struct trap_frame *regs) {
  return (regs->cs & 0x3) != 0;
}

int hal_describe_trap_frame(struct trap_frame *regs, int max_regs,
                            const char **names, uintptr_t *values) {
  if (max_regs < i686_MAX_REGS)
    return -1;
  if (!regs)
    panic_halt("describe_regs(NULL)!");

  // 1. Define names in the exact order of your struct + Control Registers
  static const char *_names[] = {
      "gs",     "fs",       "es",     "ds",        // Segment registers
      "edi",    "esi",      "ebp",    "esp_dummy", // pusha part 1
      "ebx",    "edx",      "ecx",    "eax",       // pusha part 2
      "int_no", "err_code",                        // ISR Stub
      "eip",    "cs",       "eflags", "esp_user",  "ss_user", // CPU Automatic
      "cr2",    "cr3"                                         // Contextual info
  };

  // Copy names to the output buffer
  for (int i = 0; i < i686_MAX_REGS; i++) {
    names[i] = _names[i];
  }

  // 2. Map struct members to the values array
  values[0] = regs->gs;
  values[1] = regs->fs;
  values[2] = regs->es;
  values[3] = regs->ds;
  values[4] = regs->edi;
  values[5] = regs->esi;
  values[6] = regs->ebp;
  values[7] = regs->esp_dummy;
  values[8] = regs->ebx;
  values[9] = regs->edx;
  values[10] = regs->ecx;
  values[11] = regs->eax;
  values[12] = regs->interrupt;
  values[13] = regs->error;
  values[14] = regs->eip;
  values[15] = regs->cs;
  values[16] = regs->eflags;
  values[17] = regs->esp_user;
  values[18] = regs->ss_user;

  // 3. Capture Control Registers (Very useful for Page Faults/cr2)
  __asm__ volatile("mov %%cr2, %0" : "=r"(values[19]));
  __asm__ volatile("mov %%cr3, %0" : "=r"(values[20]));

  return i686_MAX_REGS;
}

uintptr_t hal_backtrace(uintptr_t *data, struct trap_frame *regs) {
  if (*data == 0) {
    if (regs)
      *data = regs->ebp;
    else
      __asm__ volatile("mov %%ebp, %0" : "=r"(*data));
  }

  uintptr_t past_instruction_ptr = *(uintptr_t *)(*data + 4);
  *data = *(uintptr_t *)(*data - sizeof(uint32_t));

  if (*data == 0)
    return 0;
  return past_instruction_ptr;
}

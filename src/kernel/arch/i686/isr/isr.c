#include "isr.h"
#include "arch/i686/idt.h"
#include "arch/i686/panic.h"
#include "include/stdio.h" // kernel printf implementation
#include "kernel_symbols.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

ISRHandler g_isr_handlers[IDT_SIZE];

static const char *const g_exceptions[] = {"Divide by zero error",
                                           "Debug",
                                           "Non-maskable Interrupt",
                                           "Breakpoint",
                                           "Overflow",
                                           "Bound Range Exceeded",
                                           "Invalid Opcode",
                                           "Device Not Available",
                                           "Double Fault",
                                           "Coprocessor Segment Overrun",
                                           "Invalid TSS",
                                           "Segment Not Present",
                                           "Stack-Segment Fault",
                                           "General Protection Fault",
                                           "Page Fault",
                                           "",
                                           "x87 Floating-Point Exception",
                                           "Alignment Check",
                                           "Machine Check",
                                           "SIMD Floating-Point Exception",
                                           "Virtualization Exception",
                                           "Control Protection Exception ",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "",
                                           "Hypervisor Injection Exception",
                                           "VMM Communication Exception",
                                           "Security Exception",
                                           ""};

#include "arch/i686/gdt.h"
#include "gen_isr_handler_declerations.inc"
void i686_isr_init() {
#include "arch/i686/isr/gen_isr_gates_sets.inc"
  for (int i = 0; i < IDT_SIZE; i++)
    i686_idt_gate_enable(i);
}

static void panic_ebp_backtrace(uintptr_t *ebp) {
  debugf_and_printf("Backtrace (EBP chain):\n");

  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 20) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    debugf_and_printf("  #%d: 0x%x <%s>\n", depth, (unsigned)ret,
                      lookup_symbol(ret));
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
}

// for than the run.py to use with addr2line tool
void debugf_stacktrace_line(uintptr_t *ebp) {
  debugf("STACK_OF_PANIC[123]:");
  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 20) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    debugf(" 0x%x", (unsigned)ret);
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
  debugf("\n");
}

void i686_isr_handler(Registers *regs) {
  if (g_isr_handlers[regs->interrupt] != NULL)
    g_isr_handlers[regs->interrupt](regs);
  else if (regs->interrupt >= 32)
    debugf_and_printf("Unhandled interrupt %d!\n", regs->interrupt);
  else {
    debugf_and_printf("Unhandled exception %d %s\n", regs->interrupt,
                      g_exceptions[regs->interrupt]);
    debugf_and_printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
                      regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi,
                      regs->edi);

    debugf_and_printf(
        "  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
        regs->esp_user, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds,
        regs->ss_user);
    debugf_and_printf("  interrupt=%x  errorcode=%x\n", regs->interrupt,
                      regs->error);

    if ((regs->cs & 0x3) == 0) {
      debugf("FAULTING_INSTRUCTION_OF_PANIC[123]: 0x%x\n", (unsigned)regs->eip);

      debugf_stacktrace_line((uintptr_t *)regs->ebp);
      panic_ebp_backtrace((uintptr_t *)regs->ebp);

    } else {
      debugf("User space exception at eip=0x%x\n", regs->eip);
      debugf("Cannot trace user stack from kernel\n");
    }
    arch_panic_halt();
  }
}

void i686_isr_handler_register(int interrupt, ISRHandler handler) {
  g_isr_handlers[interrupt] = handler;
  i686_idt_gate_enable(interrupt);
}

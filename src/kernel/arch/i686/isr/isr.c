#include "isr.h"
#include "arch/i686/idt.h"
#include "arch/i686/io.h"
#include <include/stdio.h>
#include <stddef.h>

ISRHandler g_isr_handlers[IDT_SIZE];

static const char* const g_exceptions[] = {"Divide by zero error",
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

void __attribute__((cdecl)) i686_isr_handler(Registers* regs) {
    if (g_isr_handlers[regs->interrupt] != NULL)
        g_isr_handlers[regs->interrupt](regs);

    else if (regs->interrupt >= 32)
        printf("Unhandled interrupt %d!\n", regs->interrupt);

    else {
        printf("Unhandled exception %d %s\n", regs->interrupt,
               g_exceptions[regs->interrupt]);

        printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n", regs->eax,
               regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

        printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs,
               regs->ds, regs->ss);

        printf("  interrupt=%x  errorcode=%x\n", regs->interrupt, regs->error);

        printf("KERNEL PANIC!\n");
        i686_panic();
    }
}

void i686_isr_handler_register(int interrupt, ISRHandler handler) {
    g_isr_handlers[interrupt] = handler;
    i686_idt_gate_enable(interrupt);
}

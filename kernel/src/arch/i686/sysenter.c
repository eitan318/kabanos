/**
 * @file sysenter.c
 * @brief SYSENTER MSR setup for fast syscalls.
 */
#include "arch/i686/gdt.h"
#include "arch/i686/msr.h"
#include "sched/dispatcher.h"
#include "sched/thread.h"

extern void sysenter_handler_entry();

void i686_sysenter_init() {
  thread_t *curr = dispatch_get_current();
  wrmsr(MSR_IA32_SYSENTER_CS, i686_GDT_KERNEL_CS_SEL);
  // The esp, will be updated on each context switch
  // wrmsr(MSR_IA32_SYSENTER_ESP, (uint32_t)curr->kstack_top);
  wrmsr(MSR_IA32_SYSENTER_EIP, (uint32_t)sysenter_handler_entry);
}

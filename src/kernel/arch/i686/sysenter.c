#include "arch/i686/gdt.h"
#include "arch/i686/msr.h"
#include "sched/thread.h"

extern void _sysenter_handler_entry();
extern thread_t g_current_thread;

void i686_sysenter_init() {
  wrmsr(MSR_IA32_SYSENTER_CS, i686_GDT_KERNEL_CS_SEL);
  // The esp, will be updated on each context switch
  wrmsr(MSR_IA32_SYSENTER_ESP, (uint32_t)g_current_thread.kstack_top);
  wrmsr(MSR_IA32_SYSENTER_EIP, (uint32_t)_sysenter_handler_entry);
}

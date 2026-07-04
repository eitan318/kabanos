/**
 * @file hal_kernel_stack.c
 * @brief Points TSS.esp0 and the SYSENTER stack at a thread's kernel stack.
 */
#include "arch/i686/gdt.h"
#include "arch/i686/msr.h"
#include "arch/i686/tss.h"
#include "hal.h"

void hal_update_tss_and_syssenter_kstack(int cpu_id, void *kstack_top) {
  wrmsr(MSR_IA32_SYSENTER_ESP, (uint32_t)kstack_top);

  tss_entry_t *curr_tss = tss_entry_get(cpu_id);
  curr_tss->ss0 = i686_GDT_KERNEL_DS_SEL;
  curr_tss->esp0 = (uint32_t)(kstack_top);
}

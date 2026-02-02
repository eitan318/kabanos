#include "gdt.h"
#include "hal.h"
#include "msr.h"
#include "tss.h"

void hal_update_kernel_stack(int cpu_id, void *kstack_top) {
  wrmsr(MSR_IA32_SYSENTER_ESP, (uint32_t)kstack_top);

  tss_entry_t *curr_tss = tss_entry_get(cpu_id);
  curr_tss->ss0 = i686_GDT_KERNEL_DS_SEL;
  curr_tss->esp0 = (uint32_t)(kstack_top);
}

#include "hal.h"
#include "isr.h"
#include "modules/modules.h"
#include "panic.h"
#include "sched/dispatcher.h"
#include "stdio.h"

void pf_handle(trap_frame_t *regs) {
  uintptr_t addr;
  // On x86, the faulting address is in CR2
  asm volatile("mov %%cr2, %0" : "=r"(addr));

  // Get the current VM space (usually from the current thread/process)
  arch_vm_t *curr_vm = dispatch_get_current()->process->vmspace->arch;

  // Try to resolve as COW.
  // Pass the error_code so the handler can check if it was a WRITE.
  if (hal_vmm_handle_cow(curr_vm, addr)) {
    // COW resolved successfully!
    // Returning from the ISR will retry the instruction that faulted.
    return;
  }

  // If we reach here, it wasn't a COW fault.
  // It's a real Page Fault (Null pointer, permission violation, etc.)
  kdebugf_and_printf("Fatal Page Fault at 0x%p (Error Code: 0x%x)\n", addr, 14);

  // Fall back to the kernel's standard panic or process killer
  panic_from_regs(regs);
}

int pf_handler_init(module_t *self) {
  isr_handler_register(14, pf_handle);
  return 0;
}

static const char *pf_handler_deps[] = {"hal", "dispatcher", NULL};

ITER_MODULE(keyboard) = {
    .name = "pf_handler",
    .required = pf_handler_deps,
    .init = &pf_handler_init,
    .fini = NULL,
};

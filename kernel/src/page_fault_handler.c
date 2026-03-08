#include "hal.h"
#include "isr.h"
#include "klib/stdio.h"
#include "modules.h"
#include "panic.h"
#include "sched/dispatcher.h"

void pf_handle(trap_frame_t *regs) {
  uintptr_t addr;
  // On x86, the faulting address is in CR2
  asm volatile("mov %%cr2, %0" : "=r"(addr));

  arch_vm_t *curr_vm = dispatch_get_current()->process->vmspace->arch;

  if (hal_vmm_handle_cow(curr_vm, addr)) {
    return;
  }

  kdebugf_and_printf("Fatal Page Fault at 0x%p (Error Code: 0x%x)\n", addr, 14);
  panic_from_regs(regs);
}

int pf_handler_init(module_t *self) {
  isr_handler_register(14, pf_handle);
  return 0;
}

static const char *pf_handler_deps[] = {"hal", "dispatcher", NULL};

ITER_MODULE(keyboard) = {
    .name = "pf_handler",
    .required_modules_names = pf_handler_deps,
    .init = &pf_handler_init,
    .fini = NULL,
};

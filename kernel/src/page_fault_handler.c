#include "hal.h"
#include "isr.h"
#include "klib/stdio.h"
#include "mm/va_allocation.h"
#include "modules.h"
#include "panic.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include <mm/vmspace.h>

enum pf_errors {
  PF_ERR_PRESENT = (1 >> 0),
  PF_ERR_WRITE = (1 >> 1),
  PF_ERR_USER = (1 >> 2),
  PF_ERR_RESERVED_WRITE = (1 >> 3),
  PF_ERR_INSTRUCTION_FETCH = (1 >> 4),
  PF_ERR_PROTECTION_KEY = (1 >> 5),
};

void pf_handle(trap_frame_t *regs) {
  uintptr_t addr;
  asm volatile("mov %%cr2, %0" : "=r"(addr));

  vmspace_t *curr_vm = dispatch_get_current()->process->vmspace;
  vma_t *addr_vma = vmspace_find_vma(curr_vm, addr);

  if (addr_vma == NULL) {
    kprintf("Segmentation Fault: No VMA at 0x%p\n", addr);
    panic_from_regs(regs);
    return;
  }

  // Write to read-only VMA
  if ((regs->error & PF_ERR_WRITE) &&
      !(addr_vma->flags & VMA_WRITE)) { // fix: VMA_WRITE not VMA_READ
    kprintf("Segmentation Fault: Write to Read-Only VMA at 0x%p\n", addr);
    panic_from_regs(regs);
    return;
  }

  if (hal_vmm_handle_cow(curr_vm->arch, addr)) {
    return;
  }

  if (!(regs->error & PF_ERR_PRESENT)) {
    uintptr_t page_addr = addr & ~0xFFF;
    uint32_t page_flags = 0;
    if (addr_vma->flags & VMA_READ)
      page_flags |= PAGE_PRESENT;
    if (addr_vma->flags & VMA_WRITE)
      page_flags |= PAGE_READWRITE;
    if (addr_vma->flags & VMA_USER)
      page_flags |= PAGE_USER;

    kprintf("pfhandler\n");
    if (va_alloc_region(curr_vm->arch, page_addr, PAGE_SIZE, page_flags)) {
      return;
    }

    kprintf("Out of Memory: Failed to allocate frame for 0x%p\n", addr);
    panic_from_regs(regs);
    return;
  }

  // fix: moved outside the PF_ERR_PRESENT block — unhandled fault that is
  // present-but-not-cow
  kprintf("Unhandled Page Fault at 0x%p (Error: 0x%x)\n", addr, regs->error);
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

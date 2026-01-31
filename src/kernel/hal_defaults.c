#include "hal.h"
#include <stddef.h>
#include <stdint.h>

#define weak __attribute__((__weak__))

// Init
void weak hal_arch_init(void) {}

// Panic
int weak hal_describe_regs(struct arch_regs *regs, int max, const char **names,
                           uintptr_t *values) {
  return -1;
}
uintptr_t weak hal_backtrace(uintptr_t *data, struct arch_regs *regs) {
  return NULL;
}

void weak hal_halt(void) {}
void weak hal_trap() {}

// Regs
int weak hal_regs_interrupt_number(struct arch_regs *regs) { return -1; }
uintptr_t weak hal_regs_pc(struct arch_regs *regs) { return NULL; }
bool weak hal_regs_from_user(const struct arch_regs *regs) { return false; }
unsigned weak hal_regs_max_get() { return -1; }

// Interrupts
void weak hal_interrupts_enable(void) {}
void weak hal_interrupts_disable(void) {}
int weak hal_interrupts_state_get() { return -1; }
const char *weak hal_exception_name(int vector) {
  return "hal_exception_name() not implemented";
}

// Debug
void weak hal_serial_putc(const char c) {}

// IO
uint8_t weak hal_in8(uint16_t port) { return -1; }
uint16_t weak hal_in16(uint16_t port) { return -1; }
void weak hal_out8(uint16_t port, uint8_t value) {}
void weak hal_out16(uint16_t port, uint16_t value) {}

// IRQ
void weak hal_irq_enable(int irq) {}
void weak hal_irq_disable(int irq) {}
void weak hal_irq_send_eoi(uint8_t irq) {}

// VMM
bool weak hal_vm_map(page_dir_t *pd, vaddr_t va, paddr_t pa, uint32_t flags) {
  return false;
}
bool weak hal_vm_unmap(page_dir_t *pd, vaddr_t va) { return false; }
paddr_t weak hal_vm_virt_to_phys(page_dir_t *pd, vaddr_t va) { return -1; }
bool weak hal_vm_map_range(page_dir_t *pd_virt, paddr_t pa_start,
                           vaddr_t va_start, size_t size, uint32_t flags) {
  return false;
}
bool weak hal_vm_unmap_range(page_dir_t *pd_virt, vaddr_t va_start,
                             size_t size) {
  return false;
}
paddr_t weak hal_vm_empty_pd_create() { return -1; }
void weak hal_vm_pd_destroy(page_dir_t *pd) {}

// Processes
void weak hal_set_kernel_stack(int cpu_id, void *kstack_top) {}
void *weak hal_build_initial_frame(void *kstack_top, uintptr_t entry,
                                   uintptr_t user_stack, enum thread_mode mode,
                                   int interrupt_number) {
  return NULL;
}

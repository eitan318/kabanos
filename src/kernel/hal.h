#pragma once
#include "arch/types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct arch_regs;
typedef void (*interrupt_handler_t)(struct arch_regs *r);

// Init
void hal_arch_init(void);

// Panic
int hal_describe_regs(struct arch_regs *regs, int max, const char **names,
                      uintptr_t *values);
uintptr_t hal_backtrace(uintptr_t *data, struct arch_regs *regs);
void hal_halt(void);
void hal_trap();

// Regs
int hal_regs_interrupt_number(struct arch_regs *regs);
uintptr_t hal_regs_pc(struct arch_regs *regs);
bool hal_regs_from_user(const struct arch_regs *regs);
unsigned hal_regs_max_get();

// Interrupts
void hal_interrupts_enable(void);
void hal_interrupts_disable(void);
int hal_interrupts_state_get();

const char *hal_exception_name(int vector);

// Debug
void hal_serial_putc(const char c);

// IO
uint8_t hal_in8(uint16_t port);
uint16_t hal_in16(uint16_t port);
void hal_out8(uint16_t port, uint8_t value);
void hal_out16(uint16_t port, uint16_t value);

// IRQ
void hal_irq_enable(int irq);
void hal_irq_disable(int irq);
void hal_irq_send_eoi(uint8_t irq);

typedef uint32_t vaddr_t;
typedef uint32_t paddr_t;

#define PAGE_PRESENT 0x1
#define PAGE_READWRITE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)
//
// VMM
//
//  - mappings
bool hal_vm_map(arch_vm_t *pd, vaddr_t va, paddr_t pa, uint32_t flags);
bool hal_vm_unmap(arch_vm_t *pd, vaddr_t va);
paddr_t hal_vm_virt_to_phys(arch_vm_t *pd, vaddr_t va);
bool hal_vm_map_range(arch_vm_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t flags);
bool hal_vm_unmap_range(arch_vm_t *pd_virt, vaddr_t va_start, size_t size);

//  - context
bool hal_vm_empty_arch_vm_create(arch_vm_t *kernel_arch_vm);
void hal_vm_arch_clone(arch_vm_t *dst, arch_vm_t *src);
void hal_vm_arch_load(arch_vm_t *arch_vm);
void hal_vm_arch_destroy(arch_vm_t *vm);

// Processes
enum thread_mode { THREAD_MODE_KERNEL, THREAD_MODE_USER };
void hal_set_kernel_stack(int cpu_id, void *kstack_top);
void *hal_build_initial_frame(void *kstack_top, uintptr_t entry,
                              uintptr_t user_stack, enum thread_mode mode,
                              int interrupt_number);

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer (HAL) interface.
 *
 * Architecture-neutral entry points implemented by the active arch port
 * (currently i686). Covers CPU bring-up, interrupts, port I/O, virtual
 * memory, thread context switching and the system timer.
 */
#pragma once
#include "arch/types.h"
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"
#include "modules.h"
#include "sched/thread.h"

/** @brief Handler invoked for a registered interrupt vector. */
typedef void (*interrupt_handler_t)(trap_frame_t *r);

/* --- Init --- */

/** @brief Module entry point: initializes the architecture (GDT, IDT, ...). */
int hal_arch_init(module_t *self);

/* --- Panic support --- */

/**
 * @brief Extracts register names/values from a trap frame for display.
 * @param regs Trap frame to describe.
 * @param max Capacity of the @p names and @p values arrays.
 * @param names [out] Register name per entry.
 * @param values [out] Register value per entry.
 * @return Number of entries filled.
 */
int hal_describe_trap_frame(trap_frame_t *regs, int max, const char **names,
                            uintptr_t *values);

/**
 * @brief Walks stack frames starting from @p regs.
 * @param data [out] Receives return addresses, one per frame.
 * @param regs Trap frame to start the walk from.
 * @return Number of frames written.
 */
uintptr_t hal_backtrace(uintptr_t *data, trap_frame_t *regs);

/** @brief Stops the CPU permanently. */
void hal_halt(void);

/** @brief Triggers a software trap (useful for debugging). */
void hal_trap();

/* --- Trap frame accessors --- */

/** @brief Returns the interrupt vector number stored in @p regs. */
int hal_regs_interrupt_number(trap_frame_t *regs);

/** @brief Returns the program counter (EIP) stored in @p regs. */
uintptr_t hal_regs_pc(trap_frame_t *regs);

/** @brief True if the trap was taken while executing user-mode code. */
bool hal_regs_from_user(const trap_frame_t *regs);

/** @brief Number of registers hal_describe_trap_frame can report. */
unsigned hal_regs_max_get();

/* --- Interrupts --- */

void hal_interrupts_enable(void);
void hal_interrupts_disable(void);

/** @brief Returns nonzero if interrupts are currently enabled. */
int hal_interrupts_state_get();

/** @brief Human-readable name of a CPU exception vector. */
const char *hal_exception_name(int vector);

/* --- Debug --- */

/** @brief Writes one character to the debug serial port. */
void hal_serial_putc(const char c);

/* --- Port I/O --- */

uint8_t hal_in8(uint16_t port);
uint16_t hal_in16(uint16_t port);
uint32_t hal_in32(uint16_t port);
void hal_out8(uint16_t port, uint8_t value);
void hal_out16(uint16_t port, uint16_t value);
void hal_out32(uint16_t port, uint32_t value);

/* --- IRQ (interrupt controller) --- */

/** @brief Unmasks the given IRQ line. */
void hal_irq_enable(int irq);
/** @brief Masks the given IRQ line. */
void hal_irq_disable(int irq);
/** @brief Signals end-of-interrupt to the controller for @p irq. */
void hal_irq_send_eoi(uint8_t irq);

typedef uint32_t vaddr_t; /**< Virtual address. */
typedef uint32_t paddr_t; /**< Physical address. */

/* Page mapping flags for hal_vm_map*() */
#define PAGE_PRESENT 0x1
#define PAGE_READWRITE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

/* --- VMM: mappings --- */

/** @brief Maps one page @p va -> @p pa with the given PAGE_* flags. */
bool hal_vm_map(arch_vm_t *pd, vaddr_t va, paddr_t pa, uint32_t flags);

/** @brief Removes the mapping for @p va. */
bool hal_vm_unmap(arch_vm_t *pd, vaddr_t va);

/** @brief Translates a virtual address to its physical address. */
paddr_t hal_vm_virt_to_phys(arch_vm_t *pd, vaddr_t va);

/** @brief Maps a contiguous physical range at @p va_start. */
bool hal_vm_map_range(arch_vm_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t flags);

/** @brief Unmaps @p size bytes starting at @p va_start. */
bool hal_vm_unmap_range(arch_vm_t *pd_virt, vaddr_t va_start, size_t size);

/** @brief Copies all mappings from @p src into @p dst. */
void hal_vm_arch_clone_mapping(arch_vm_t *dst, arch_vm_t *src);

/* --- VMM: address-space lifecycle --- */

/** @brief Creates a fresh address space containing only kernel mappings. */
bool hal_vm_empty_arch_vm_create(arch_vm_t *kernel_arch_vm);

/** @brief Creates every kernel-half page table up front so kernel PDEs
 *         never change after boot. See implementation for why this
 *         matters. Call once, after the physical memory map is in place
 *         and before any process is created. */
void hal_vm_prealloc_kernel_tables(arch_vm_t *vm);

/** @brief Clones an address space (used by fork; user pages become COW). */
void hal_vm_arch_clone(arch_vm_t *dst, arch_vm_t *src);

/** @brief Activates @p arch_vm on the current CPU (loads CR3). */
void hal_vm_arch_load(arch_vm_t *arch_vm);

/** @brief Frees all structures owned by @p vm. */
void hal_vm_arch_destroy(arch_vm_t *vm);

/**
 * @brief Resolves a copy-on-write fault at @p addr.
 * @return true if the fault was a COW page and was resolved.
 */
bool hal_vmm_handle_cow(arch_vm_t *arch_vm, uintptr_t addr);

/* --- Processes --- */

/** @brief Points TSS.esp0 and the SYSENTER stack MSR at @p kstack_top. */
void hal_update_tss_and_syssenter_kstack(int cpu_id, void *kstack_top);

/* --- Threads --- */

/** @brief Prepares a new thread's kernel stack and initial trap frame. */
int hal_thread_init(thread_t *t, uintptr_t entry, uintptr_t user_stack);

/** @brief Switches CPU context from @p curr to @p next. */
void hal_thread_switch(thread_t *curr, thread_t *next);

/** @brief Copies @p parent's CPU context into @p child (fork). */
int hal_thread_clone(thread_t *parent, thread_t *child);

/** @brief Sets the value the thread will see as its syscall return. */
void hal_thread_set_return_value(thread_t *t, uint64_t val);

/** @brief Re-initializes a thread's trap frame for a fresh start (exec). */
void hal_thread_trap_frame_reset(thread_t *t, uintptr_t entry,
                                 uintptr_t user_stack);

/* --- Timer --- */

/** @brief Unmasks the timer interrupt. */
void hal_timer_enable();

/** @brief Programs the periodic timer to fire at @p frequency_hz. */
void hal_timer_init(uint32_t frequency_hz);

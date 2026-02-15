#include "memory_management/memdefs.h"
#include <stdint.h>

// Multiboot header constants
typedef enum {
  ALIGNMENT_FLAG = (1 << 0),
  MEMINFO_FLAG = (1 << 1),
} MultibootFlags;

#define FLAGS (ALIGNMENT_FLAG | MEMINFO_FLAG)
#define MULTIBOOT_MAGIC 0x1BADB002
#define CHECKSUM (-(MULTIBOOT_MAGIC + FLAGS))

#define PAGE_PRESENT 0x1
#define PAGE_READWRITE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)
#define HALF_KERNEL_PD_INDEX (KERNEL_BASE / (PD_ENTRIES * PAGE_SIZE))

// External symbols
extern void kmain(uint32_t mb2_ptr);
extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

// Multiboot header in .multiboot.data section
struct {
  uint32_t magic;
  uint32_t flags;
  uint32_t checksum;
} __attribute__((section(".multiboot.data"), aligned(4)))
multiboot_header = {MULTIBOOT_MAGIC, FLAGS, CHECKSUM};

// Bootstrap stack in .bootstrap_stack section (nobits/bss)
__attribute__((section(".bootstrap_stack"), aligned(16)))
uint8_t stack_bottom[BOOT_STACK_SIZE];

// Preallocated pages for paging in .bss section
__attribute__((section(".bss"),
               aligned(PAGE_SIZE))) static uint32_t boot_pd[PD_ENTRIES];
__attribute__((section(".bss"),
               aligned(PAGE_SIZE))) static uint32_t boot_pt[PD_ENTRIES];

__attribute__((section(".multiboot.text"))) void kernel_start(void) {
  __asm__ volatile("mov %0, %%esp"
                   :
                   : "r"(stack_bottom + BOOT_STACK_SIZE)
                   : "memory");

  // Zero bss
  extern uint8_t _bss_start[], _bss_end[];
  volatile uint8_t *p = _bss_start;
  while (p < _bss_end)
    *p++ = 0;

  uintptr_t *boot_pt_phys = (uintptr_t *)((uintptr_t)boot_pt - KERNEL_BASE);
  uintptr_t *boot_pd_phys = (uintptr_t *)((uintptr_t)boot_pd - KERNEL_BASE);
  uintptr_t kernel_end_phys = (uintptr_t)&_kernel_end - KERNEL_BASE;

  // Calculate number of pages needed
  int pages_needed = (kernel_end_phys + PAGE_SIZE - 1) / PAGE_SIZE;

  // Cap at 1023 to leave room for VGA
  if (pages_needed > 1023) {
    __asm__ volatile("cli\n");
    while (1) {
      __asm__ volatile("hlt\n");
    }
  }

  for (uint32_t i = 0; i < pages_needed; i++) {
    boot_pt_phys[i] = (i * PAGE_SIZE) | PAGE_READWRITE | PAGE_PRESENT;
  }

  // Map VGA video memory to 0xC03FF000 as "present, writable"
  boot_pt_phys[1023] = VGA_SCREEN_BUF_PHYS | PAGE_PRESENT | PAGE_READWRITE;

  // Map to lower half and higher half
  boot_pd_phys[0] = (uintptr_t)boot_pt_phys | PAGE_PRESENT | PAGE_READWRITE;
  boot_pd_phys[768] = (uintptr_t)boot_pt_phys | PAGE_PRESENT | PAGE_READWRITE;

  __asm__ volatile("mov %0, %%cr3\n" : : "r"(boot_pd_phys) : "memory");

  // Enable paging and the write-protect bit
  uint32_t cr0;
  __asm__ volatile("mov %%cr0, %0\n"
                   "or $0x80010000, %0\n"
                   "mov %0, %%cr0\n"
                   : "=r"(cr0)
                   :
                   : "memory");

  __asm__ volatile("jmp higher_half\n");
}

void higher_half(void) {
  void *mb2_ptr;
  __asm__ volatile("mov %%ebx, %0" : "=r"(mb2_ptr));

  kmain((uint32_t)mb2_ptr);

  __asm__ volatile("cli\n");
  while (1)
    __asm__ volatile("hlt\n");
}

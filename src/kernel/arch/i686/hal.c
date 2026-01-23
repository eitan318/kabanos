#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/interrupts.h"
#include "arch/i686/pic.h"
#include "arch/i686/regs.h"
#include "assert.h"
#include "panic.h"
#include "string.h"
#include <stdint.h>

void hal_interrupts_disable(void) { asm volatile("cli"); }
void hal_interrupts_enable(void) { asm volatile("sti"); }
void hal_halt(void) { asm volatile("hlt"); }
void hal_trap() { __asm__ volatile("int $3"); }

int hal_interrupts_state_get() {
  uint32_t eflags;
  __asm__ volatile("pushf; pop %0" : "=r"(eflags));
  return eflags & 0x200;
}

bool hal_regs_from_user(const struct regs *regs) {
  i686_regs_t *r = regs;
  return (r->cs & 0x3) != 0;
}

int hal_interrupt_number(struct regs *regs) {
  i686_regs_t *r = regs;
  return r->interrupt;
}
uintptr_t hal_regs_pc(struct regs *regs) {
  i686_regs_t *r = regs;
  return r->eip;
}

void hal_serial_putc(const char c) { io_write8(0x3f8, c); }

void hal_arch_init(void) {
  i686_gdt_init();
  i686_idt_init();
  i686_isr_init();
  i686_pic_init();
}

int describe_regs(i686_regs_t *regs, int max, const char **names,
                  uintptr_t *values) {
  if (max < 16)
    return -1;
  if (!regs)
    panic_halt("describe_regs(NULL)!");

  static const char *_names[] = {"eax", "ecx", "edx", "ebx",    "esi", "edi",
                                 "eip", "ebp", "esp", "eflags", "cs",  "U-esp",
                                 "cr0", "cr2", "cr3", "cr4"};
  memcpy((uint8_t *)names, (uint8_t *)_names, sizeof(const char *) * 16);

  values[0] = regs->eax;
  values[1] = regs->ecx;
  values[2] = regs->edx;
  values[3] = regs->ebx;
  values[4] = regs->esi;
  values[5] = regs->edi;
  values[6] = regs->eip;
  values[7] = regs->esp_dummy;
  values[8] = regs->esp_dummy;
  values[9] = regs->eflags;
  values[10] = regs->cs;
  values[11] = regs->esp_user;

  __asm__ volatile("mov %%cr0, %0" : "=r"(values[12]));
  __asm__ volatile("mov %%cr2, %0" : "=r"(values[13]));
  __asm__ volatile("mov %%cr3, %0" : "=r"(values[14]));
  __asm__ volatile("mov %%cr4, %0" : "=r"(values[15]));

  return 16;
}

uintptr_t backtrace(uintptr_t *data, struct regs *regs) {
  i686_regs_t *arch_regs = regs;
  if (*data == 0) {
    if (regs)
      *data = arch_regs->ebp;
    else
      __asm__ volatile("mov %%ebp, %0" : "=r"(*data));
  }

  uintptr_t ip = *(uintptr_t *)(*data + 4);
  *data = *(uintptr_t *)*data;

  if (*data == 0)
    return 0;
  return ip;
}

// typedef struct {
//   uint32_t name, type, flags, addr, offset, size, link, info, addralign,
//       entsize;
// } elf_section_header_t;
//
// typedef struct {
//   uint32_t name, value, size;
//   uint8_t info, other;
//   uint16_t shndx;
// } elf_sym_t;
//
// static elf_sym_t *symtab = NULL;
// static const char *strtab = NULL;
// static unsigned num_syms = 0;
//
// static int init_syms() {
//   extern multiboot_t mboot;
//
//   if ((mboot.flags & MBOOT_ELF_SYMS) == 0)
//     return -1;
//
//   elf_section_header_t *headers = (elf_section_header_t *)mboot.addr;
//   const char *shstrtab = (const char *)headers[mboot.shndx].addr;
//
//   for (unsigned i = 0; i < mboot.num; ++i) {
//     const char *name = shstrtab + headers[i].name;
//     if (!strcmp(name, ".symtab")) {
//       symtab = (elf_sym_t *)headers[i].addr;
//       num_syms = headers[i].size / sizeof(elf_sym_t);
//     } else if (!strcmp(name, ".strtab")) {
//       strtab = (const char *)headers[i].addr;
//     }
//   }
//   return 0;
// }
//
// const char *lookup_kernel_symbol(uintptr_t addr, int *offs) {
//   if (!symtab && init_syms() == -1)
//     return NULL;
//
//   assert(symtab);
//   for (unsigned i = 0; i < num_syms; ++i) {
//     if (addr >= symtab[i].value && addr < symtab[i].value + symtab[i].size) {
//       const char *name = &strtab[symtab[i].name];
//       *offs = addr - symtab[i].value;
//       return name;
//     }
//   }
//
//   return NULL;
// }

#include <stdint.h>

typedef struct regs {
  // in the reverse order they are pushed:
  uint32_t ds; // data segment pushed by us
  uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pusha
  uint32_t interrupt,
      error; // we push interrupt, error is pushed automatically (or our dummy)
  uint32_t eip, cs, eflags, esp_user, ss_user; // pushed automatically by CPU
} __attribute__((packed)) i686_regs_t;

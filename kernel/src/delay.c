#include "delay.h"

void ndelay(uint64_t ns, uint64_t cpu_loops_per_ns) {
  uint64_t cycles = ns * cpu_loops_per_ns;
  while (cycles--) {
    __asm__ volatile("pause"); // Prevents the compiler from deleting the loop
  }
}

/**
 * @file exceptions.c
 * @brief Exception vector names for panic reporting.
 */
#include "hal.h"

// clang-format off
static const char *const i686_exceptions[] = {
  "Divide by zero error", "Debug", "Non-maskable Interrupt", "Breakpoint",
  "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
  "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS",
  "Segment Not Present", "Stack-Segment Fault", "General Protection Fault",
  "Page Fault", "", "x87 Floating-Point Exception", "Alignment Check",
  "Machine Check", "SIMD Floating-Point Exception", "Virtualization Exception",
  "Control Protection", "", "", "", "", "", "",
  "Hypervisor Injection", "VMM Communication Exception", "Security Exception", ""
};
// clang-format on

const char *hal_exception_name(int vector) {
  if (vector < 0 ||
      vector >= sizeof(i686_exceptions) / sizeof(i686_exceptions[0]))
    return "Unknown Exception";
  return i686_exceptions[vector];
}

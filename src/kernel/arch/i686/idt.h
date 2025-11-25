#pragma once

#include <stdint.h>

typedef enum {
    IDT_FLAGS_GATE_TASK = 0x05 << 0,
    IDT_FLAGS_GATE_INT_16b = 0x06 << 0,
    IDT_FLAGS_GATE_TRAP_16b = 0x07 << 0,
    IDT_FLAGS_GATE_INT_32b = 0x0e << 0,
    IDT_FLAGS_GATE_TRAP_32b = 0x0f << 0,
    IDT_FLAGS_RING0 = 0 << 5,
    IDT_FLAGS_RING1 = 1 << 5,
    IDT_FLAGS_RING2 = 2 << 5,
    IDT_FLAGS_RING3 = 3 << 5,
    IDT_FLAGS_PRESENT = 1 << 7,
} IDT_FLAGS;

#define IDT_SIZE 256

// Functions
void i686_idt_init();
void i686_idt_gate_set(int interrupt_code, void* offset,
                       uint16_t segment_selector, uint8_t flags);
void i686_idt_gate_enable(int interrupt_code);
void i686_idt_gate_disable(int interrupt_code);

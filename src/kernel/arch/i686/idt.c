#include "idt.h"
#include "utils/binary.h"

#define IDT_SELECTOR_GDT_IDX_START 3

typedef enum {
    IDT_SELECTOR_RPL0 = 0,
    IDT_SELECTOR_RPL1 = 1,
    IDT_SELECTOR_RPL2 = 2,
    IDT_SELECTOR_RPL3 = 3,
    IDT_SELECTOR_TI_GDT = 0 << 2,
    IDT_SELECTOR_TI_LDT = 1 << 2
} IDT_SELECTOR_FLAGS;

typedef struct {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t size;
    IDTEntry* offset;
} __attribute__((packed)) IDTDescriptor;

IDTEntry g_idt[IDT_SIZE];
IDTDescriptor g_idt_descriptor = {sizeof(g_idt) - 1, g_idt};

void __attribute__((cdecl)) i686_idt_load(IDTDescriptor* idtDescriptor);

void i686_idt_load(IDTDescriptor* idt_descriptor) {
    __asm__ volatile("lidt (%0)" : : "r"(idt_descriptor));
}

void i686_idt_init() { i686_idt_load(&g_idt_descriptor); }

void i686_idt_gate_set(int interrupt_code, void* offset,
                       uint16_t segment_selector, uint8_t flags) {
    g_idt[interrupt_code].offset_low = (uint32_t)offset & 0xffff;
    g_idt[interrupt_code].segment_selector = segment_selector;
    g_idt[interrupt_code].reserved = 0;
    g_idt[interrupt_code].flags = flags;
    g_idt[interrupt_code].offset_high = ((uint32_t)offset >> 16) & 0xffff;
}

void i686_idt_gate_enable(int interrupt_code) {
    FLAG_SET(g_idt[interrupt_code].flags, IDT_FLAGS_PRESENT);
}

void i686_idt_gate_disable(int interrupt_code) {
    FLAG_UNSET(g_idt[interrupt_code].flags, IDT_FLAGS_PRESENT);
}

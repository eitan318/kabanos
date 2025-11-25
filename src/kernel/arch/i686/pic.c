#include "pic.h"
#include "arch/i686/io.h"

// Track which IRQs are masked
static uint8_t pic1_mask = 0xFF;  // Start with all masked
static uint8_t pic2_mask = 0xFF;

// Initialize the PIC (Programmable Interrupt Controller)
void pic_init() {
    // ICW1: Initialize PIC
    i686_outb(PIC1_COMMAND, 0x11);
    i686_outb(PIC2_COMMAND, 0x11);

    // ICW2: Vector offsets (IRQ 0-7 -> INT 0x20-0x27, IRQ 8-15 -> INT 0x28-0x2F)
    i686_outb(PIC1_DATA, 0x20);
    i686_outb(PIC2_DATA, 0x28);

    // ICW3: Tell Master PIC there's a slave at IRQ2, tell Slave its cascade identity
    i686_outb(PIC1_DATA, 0x04);
    i686_outb(PIC2_DATA, 0x02);

    // ICW4: 8086 mode
    i686_outb(PIC1_DATA, 0x01);
    i686_outb(PIC2_DATA, 0x01);

    // Mask all interrupts initially
    i686_outb(PIC1_DATA, pic1_mask);
    i686_outb(PIC2_DATA, pic2_mask);
}

// Unmask (enable) a specific IRQ
void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    
    if (irq < 8) {
        port = PIC1_DATA;
        pic1_mask &= ~(1 << irq);
        i686_outb(port, pic1_mask);
    } else {
        port = PIC2_DATA;
        pic2_mask &= ~(1 << (irq - 8));
        i686_outb(port, pic2_mask);
    }
}

// Mask (disable) a specific IRQ
void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    
    if (irq < 8) {
        port = PIC1_DATA;
        pic1_mask |= (1 << irq);
        i686_outb(port, pic1_mask);
    } else {
        port = PIC2_DATA;
        pic2_mask |= (1 << (irq - 8));
        i686_outb(port, pic2_mask);
    }
}

// Send End of Interrupt
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        i686_outb(PIC2_COMMAND, PIC_EOI);
    }
    i686_outb(PIC1_COMMAND, PIC_EOI);
}
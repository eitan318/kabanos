#include "arch/i686/pic.h"
#include "hal.h"

// Initialize the PIC (Programmable Interrupt Controller)
void i686_pic_init() {
  // ICW1: Initialize PIC
  hal_out8(PIC1_COMMAND, 0x11);
  hal_out8(PIC2_COMMAND, 0x11);

  // ICW2: Vector offsets (IRQ 0-7 -> INT 0x20-0x27, IRQ 8-15 -> INT 0x28-0x2F)
  hal_out8(PIC1_DATA, 0x20);
  hal_out8(PIC2_DATA, 0x28);

  // ICW3: Tell Master PIC there's a slave at IRQ2, tell Slave its cascade
  // identity
  hal_out8(PIC1_DATA, 0x04);
  hal_out8(PIC2_DATA, 0x02);

  // ICW4: 8086 mode
  hal_out8(PIC1_DATA, 0x01);
  hal_out8(PIC2_DATA, 0x01);

  i686_pic_disable();
}

// Unmask (enable) a specific IRQ
void i686_pic_unmask_irq(uint8_t irq) {
  uint16_t port;
  uint8_t value;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    // For slave PIC (IRQ 8-15), we MUST also unmask IRQ 2 on master
	  // because the slave connected to it - see init()
    port = PIC2_DATA;
    irq -= 8;
    
    value = hal_in8(PIC1_DATA);
    hal_out8(PIC1_DATA, value & ~(1 << 2));
  }
  
  value = hal_in8(port);
  hal_out8(port, value & ~(1 << irq));
}


// Mask (disable) a specific IRQ
void i686_pic_mask_irq(uint8_t irq) {
  uint16_t port;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
    irq -= 8;
  }

  uint8_t mask = hal_in8(port);
  hal_out8(port, mask | (1 << irq));
}

// Send End of Interrupt
void i686_pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    hal_out8(PIC2_COMMAND, PIC_EOI);
  }
  hal_out8(PIC1_COMMAND, PIC_EOI);
}

void i686_pic_disable() {
  // Mask all interrupts to disable PICs
  hal_out8(PIC1_DATA, 0xff);
  hal_out8(PIC2_DATA, 0xff);
}

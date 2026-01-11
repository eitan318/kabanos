#include "pic.h"
#include "hal/io.h"

// Initialize the PIC (Programmable Interrupt Controller)
void pic_init() {
  // ICW1: Initialize PIC
  io_write8(PIC1_COMMAND, 0x11);
  io_write8(PIC2_COMMAND, 0x11);

  // ICW2: Vector offsets (IRQ 0-7 -> INT 0x20-0x27, IRQ 8-15 -> INT 0x28-0x2F)
  io_write8(PIC1_DATA, 0x20);
  io_write8(PIC2_DATA, 0x28);

  // ICW3: Tell Master PIC there's a slave at IRQ2, tell Slave its cascade
  // identity
  io_write8(PIC1_DATA, 0x04);
  io_write8(PIC2_DATA, 0x02);

  // ICW4: 8086 mode
  io_write8(PIC1_DATA, 0x01);
  io_write8(PIC2_DATA, 0x01);

  pic_disable();
}

// Unmask (enable) a specific IRQ
void pic_unmask_irq(uint8_t irq) {
  uint16_t port;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
	irq -=8;
  }
  
  uint8_t unmask = io_read8(port);
  io_write8(port, unmask & ~(1 << irq));
}

// Mask (disable) a specific IRQ
void pic_mask_irq(uint8_t irq) {
  uint16_t port;

  if (irq < 8) {
    port = PIC1_DATA;
  } else {
    port = PIC2_DATA;
	irq -= 8;
  }
  
  uint8_t mask = io_read8(port);
  io_write8(port, mask | (1 << irq));
}

// Send End of Interrupt
void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    io_write8(PIC2_COMMAND, PIC_EOI);
  }
  io_write8(PIC1_COMMAND, PIC_EOI);
}

void pic_disable() {
  // Mask all interrupts to disable PICs
  io_write8(PIC1_DATA, 0xff);
  io_write8(PIC2_DATA, 0xff);
}


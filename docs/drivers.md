5. Step 2: Abstract interrupts (this is the big one)
Generic interrupt API
// hal/interrupt.h
typedef void (*irq_handler_t)(void *ctx);

void irq_register(int irq, irq_handler_t handler, void *ctx);
void irq_enable(int irq);

i686 implementation
// arch/i686/interrupt.c
void irq_register(int irq, irq_handler_t handler, void *ctx) {
    i686_isr_handler_register(irq_to_vector(irq), handler, ctx);
}

void irq_enable(int irq) {
    pic_unmask_irq(irq);
}

RTL8139 now becomes arch-agnostic
#include "hal/interrupt.h"

irq_register(dev->irq, rtl8139_isr_handler, dev);
irq_enable(dev->irq);


No PIC. No ISR vectors. No i686 includes.

6. Step 3: Remove hardcoded IRQs and base address
❌ Current
#define RTL8139_BASE_ADDR 0xC000
#define RTL8139_IRQ 11

✅ Correct (PCI-discovered)
typedef struct {
    uintptr_t io_base;
    int irq;
    uint8_t mac[6];
    void *rx_buffer;
    uint32_t rx_offset;
} rtl8139_device_t;


Passed in by PCI layer:

rtl8139_init(&dev);

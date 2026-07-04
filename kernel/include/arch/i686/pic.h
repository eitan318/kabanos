/**
 * @file pic.h
 * @brief 8259A Programmable Interrupt Controller driver.
 */
#pragma once

#include "klib/stdint.h"

/* I/O ports of the master (PIC1) and slave (PIC2) controllers */
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20 /**< End-of-interrupt command code. */

/** @brief Remaps the PICs so IRQs 0-15 land on vectors 32-47. */
void i686_pic_init();

/** @brief Unmasks (enables) the given IRQ line. */
void i686_pic_unmask_irq(uint8_t irq);

/** @brief Masks (disables) the given IRQ line. */
void i686_pic_mask_irq(uint8_t irq);

/** @brief Acknowledges an IRQ; also notifies the slave PIC for IRQ >= 8. */
void i686_pic_send_eoi(uint8_t irq);

/** @brief Masks all IRQ lines on both PICs. */
void i686_pic_disable();

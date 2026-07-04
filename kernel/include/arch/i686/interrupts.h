/**
 * @file interrupts.h
 * @brief IDT and ISR stub installation for i686.
 */
#pragma once

/** @brief Installs the ISR/IRQ stubs into the IDT. */
void i686_isr_init(void);

/** @brief Builds the IDT and loads it with lidt. */
void i686_idt_init(void);

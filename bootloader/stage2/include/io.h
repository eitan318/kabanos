/**
 * @file io.h
 * @brief Port I/O and panic primitives (implemented in assembly).
 */
#pragma once
#include <stdint.h>

uint8_t __attribute__((cdecl)) i686_inb(uint16_t port);
uint16_t __attribute__((cdecl)) i686_inw(uint16_t port);
uint32_t __attribute__((cdecl)) i686_inl(uint16_t port);
void __attribute__((cdecl)) i686_outb(uint16_t port, uint8_t value);
void __attribute__((cdecl)) i686_outw(uint16_t port, uint16_t value);
void __attribute__((cdecl)) i686_outl(uint16_t port, uint32_t value);
void __attribute__((cdecl)) i686_panic(void);

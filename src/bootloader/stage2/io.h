#pragma once

#include <stdint.h>

// Input a byte from a port
static inline uint8_t i686_inb(uint16_t port) {
  uint8_t value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

// Input a word from a port
static inline uint16_t i686_inw(uint16_t port) {
  uint16_t value;
  __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

// Input a dword from a port
static inline uint32_t i686_inl(uint16_t port) {
  uint32_t value;
  __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

// Output a byte to a port
static inline void i686_outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Output a word to a port
static inline void i686_outw(uint16_t port, uint16_t value) {
  __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

// Output a dword to a port
static inline void i686_outl(uint16_t port, uint32_t value) {
  __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline void i686_panic() { __asm__ volatile("cli; hlt"); }

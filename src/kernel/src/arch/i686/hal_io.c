#include <stdint.h>

uint8_t hal_in8(uint16_t port) {
  uint8_t value;
  __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

uint16_t hal_in16(uint16_t port) {
  uint16_t value;
  __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

uint32_t io_read32(uint16_t port) {
  uint32_t value;
  __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

void hal_out8(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void hal_out16(uint16_t port, uint16_t value) {
  __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

void io_write32(uint16_t port, uint32_t value) {
  __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

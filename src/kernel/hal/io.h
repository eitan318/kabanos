#pragma once
#include <stdint.h>

uint8_t io_read8(uint16_t port);
uint16_t io_read16(uint16_t port);
void io_write8(uint16_t port, uint8_t value);
void io_write16(uint16_t port, uint16_t value);

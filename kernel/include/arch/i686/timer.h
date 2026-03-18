#pragma once
#include "klib/stdint.h"

void i686_timer_init(uint32_t frequency_hz);
uint32_t timer_get_ticks(void);
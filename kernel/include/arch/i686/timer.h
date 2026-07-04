/**
 * @file timer.h
 * @brief PIT (programmable interval timer) driver.
 */
#pragma once
#include "klib/stdint.h"

/** @brief Programs the PIT to fire IRQ0 at @p frequency_hz. */
void i686_timer_init(uint32_t frequency_hz);

/** @brief Returns the number of timer ticks since boot. */
uint32_t timer_get_ticks(void);

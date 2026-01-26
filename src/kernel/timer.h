#pragma once
#include "hal.h"

#define TIMER_IRQ 0
#define TIMER_INTERRUPT 32

void timer_isr_handler(struct regs *regs);
void timer_init();


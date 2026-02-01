#pragma once
#include "hal.h"
void isr_handler_register(uint32_t interrupt_num, interrupt_handler_t handler);
void isr_dispatch(struct arch_regs *regs);

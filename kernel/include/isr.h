/**
 * @file isr.h
 * @brief Interrupt service routine registration and dispatch.
 */
#pragma once
#include "hal.h"

/** @brief Registers @p handler to be called for @p interrupt_num. */
void isr_handler_register(uint32_t interrupt_num, interrupt_handler_t handler);

/**
 * @brief Common entry point called by the arch layer for every interrupt;
 *        invokes the registered handler for the frame's vector.
 */
void isr_dispatch(trap_frame_t *regs);

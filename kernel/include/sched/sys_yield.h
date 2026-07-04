/**
 * @file sys_yield.h
 * @brief Voluntary CPU yield syscall.
 */
#pragma once

/** @brief Gives up the rest of the time slice to the next ready thread. */
void sys_yield();

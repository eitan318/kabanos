/**
 * @file sys_sbrk.h
 * @brief Heap-growth syscall.
 */
#pragma once
#include "klib/stddef.h"

/**
 * @brief Grows (or queries, with increment 0) the process heap.
 * @return The previous break address, or (uintptr_t)-1 on failure.
 */
uintptr_t sys_sbrk(intptr_t increment);

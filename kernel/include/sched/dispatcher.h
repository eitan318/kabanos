/**
 * @file dispatcher.h
 * @brief Low-level thread dispatch: tracks the running thread and performs
 *        the actual switches (the scheduler decides, the dispatcher acts).
 */
#pragma once
#include "thread.h"

/**
 * @brief Switches to @p next from interrupt context, saving the
 *        interrupted thread's state from @p context first.
 */
void dispatch_switch_preserve_context(void *context, thread_t *next);

/** @brief Switches from the current thread to @p next (cooperative path). */
void dispatch_switch_to(thread_t *next);

/** @brief Starts the very first thread; does not save any previous state. */
void dispatch_switch_first(thread_t *next);

/** @brief Returns the thread currently running on this CPU. */
thread_t *dispatch_get_current(void);

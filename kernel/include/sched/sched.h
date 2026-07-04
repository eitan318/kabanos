/**
 * @file sched.h
 * @brief Thread Scheduler Interface.
 *
 * This file defines the core functions for the kernel scheduler. It handles
 * the management of the ready queue, selection of the next thread to run,
 * and the mechanics of context switching.
 */

#pragma once
#include "sched/thread.h"

/**
 * @brief Triggers an immediate context switch to the next available thread.
 *
 * This function is the high-level entry point for switching execution. It
 * calls the internal logic to pick a thread and then utilizes the HAL to
 * perform the actual hardware state swap.
 */
void sched_switch_next();

/**
 * @brief Adds a thread to the scheduler's ready queue.
 *
 * @param t The thread to be made eligible for CPU time.
 *
 * Transitions a thread's state to READY and places it into the appropriate
 * scheduling bucket based on the current policy.
 */
void sched_enqueue(thread_t *t);

/**
 * @brief Removes a thread from the scheduler's ready queue.
 *
 * @param t The thread to remove from consideration.
 *
 * Used when a thread blocks on I/O, sleeps, or is terminated, ensuring the
 * scheduler does not attempt to switch to it.
 */
void sched_dequeue(thread_t *t);

/**
 * @brief Selects the best candidate thread to run next according to policy.
 *
 * @return thread_t* Pointer to the chosen thread, or NULL if no threads are
 * ready.
 *
 * This function encapsulates the scheduling algorithm (e.g., Round Robin). It
 * inspects the ready queues and returns the thread that should be loaded next.
 */
thread_t *sched_pick_next(void);

/**
 * @brief Periodic entry point for the scheduler, driven by the hardware timer.
 *
 * @param context The CPU state/registers saved when the timer interrupt
 * fired.
 *
 * Updates the current thread's accounting (e.g., CPU usage, time-slice). If
 * the thread's quantum has expired, this function typically calls
 * sched_switch_next() to enforce preemption.
 */
void sched_on_timer_tick(void *context);

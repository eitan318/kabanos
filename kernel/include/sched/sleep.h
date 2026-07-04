/**
 * @file sleep.h
 * @brief Timed sleeping: sleeper list and sleep syscalls.
 */
#pragma once
#include "klib/time.h"
#include "sched/thread.h"

/** @brief Called on every timer tick; re-enqueues sleepers whose wakeup
 *         time has passed. */
void wake_up_sleeping(uint32_t g_curr_tick);

/** @brief Blocks @p t until the given tick. */
void enqueue_sleeper(thread_t *t, uint32_t wake_up_time);

/** @brief Syscall: sleeps for the given number of seconds. */
void sys_sleep(uint32_t seconds);

/** @brief Syscall: sleeps for the duration in @p req (POSIX-shaped;
 *         @p rem is currently not filled). */
int sys_nanosleep(const timespec_t *req, timespec_t *rem);

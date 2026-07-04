/**
 * @file sys_wait.h
 * @brief waitpid syscall.
 */
#pragma once
#define WNOHANG 1 /**< Return immediately if no child has exited. */
#include "proc/proc.h"

/**
 * @brief Waits for a child process to change state.
 * @param target_pid PID to wait for, or -1 for any child.
 * @param wstatus [out] Receives the child's exit status (may be NULL).
 * @param options 0 or WNOHANG.
 * @return PID of the reaped child, 0 with WNOHANG if none exited yet,
 *         negative errno on failure.
 */
pid_t sys_waitpid(pid_t target_pid, int *wstatus, int options);

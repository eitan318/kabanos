/**
 * @file sys_proc.h
 * @brief Process lifecycle syscalls: fork, exit, getpid.
 */

/**
 * @brief Clones the current process.
 * @return In parent: PID of child. In child: 0. On failure: negative error.
 */
long sys_fork();

/**
 * @brief Terminates the current process.
 * @param status The exit code to return to the parent.
 */
void sys_exit(int status);

/**
 * @brief Gets the PID of the calling process.
 * @return The current PID.
 */
long sys_getpid();

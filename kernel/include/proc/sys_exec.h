/**
 * @file exec.h
 * @brief Process execution and ELF loading logic.
 */

#pragma once
#include "sched/thread.h"

// int process_exec_noreturn(const char *path, enum thread_priority p);

/**
 * @brief Spawns a new process from a file.
 * * Creates a new process and main thread, loads the ELF, and enqueues it.
 * * @param path Path to the executable.
 * @param argc Argument count.
 * @param argv Argument array.
 * @param envp Environment array.
 * @param p Priority of the main thread.
 * @return 0 on success, -1 on failure.
 */
int process_spawn(const char *path, int argc, char *const argv[],
                  char *const envp[], enum thread_priority p);

/**
 * @brief The execve() system call.
 * * Replaces the current process image. Captures arguments, loads the new ELF,
 * switches the address space, and reinitializes the thread context.
 * * @param pathname Path to the new executable.
 * @param argv Argument array.
 * @param envp Environment array.
 * @return 0 on success, or error code on failure.
 */
long sys_execve(const char *pathname, char *const argv[], char *const envp[]);

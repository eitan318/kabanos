#pragma once
#include "sched/thread.h"

int process_exec_noreturn(const char *path, enum thread_priority p);

int process_spawn(const char *path, int argc, char *const argv[],
                  char *const envp[], enum thread_priority p);

/**
 * @brief Replaces the current process image with a new process image.
 *
 * This system call loads a new executable from the filesystem, creates a fresh
 * virtual address space, and transitions the current thread to the new program
 * entry point.
 *
 * @param[in] pathname Path to the executable file.
 * @param[in] argv     Null-terminated array of argument strings.
 * @param[in] envp     Null-terminated array of environment variable strings.
 * @return Returns 0 on success, or a negative error code (e.g., -ENOMEM,
 * -ENOENT).
 */
long sys_execve(const char *pathname, char *const argv[], char *const envp[]);

#pragma once
#include "klib/stdarg.h"
#include "klib/stdint.h"
#include "vfs.h"

// File-based character/string output
void kfputc(char c, fd_t file);
void kfputs(const char *s, fd_t file);

// Standard output character/string output
void kputc(char c);
void kputs(const char *s);

// Debug output character/string output
void kdebugc(char c);
void kdebugs(const char *s);

// ============================================================================
// Formatted Output Functions
// ============================================================================

// File-based formatted output
void kfprintf(fd_t file, const char *fmt, ...);
void kvfprintf(fd_t file, const char *format, va_list args);

// Standard output formatted output
void kprintf(const char *fmt, ...);

// Debug formatted output
void kdebugf(const char *fmt, ...);

void kdebugf_and_printf(const char *fmt, ...);

// String-based formatted output
int ksprintf(char *buffer, const char *fmt, ...);
int kvsprintf(char *buffer, const char *format, va_list args);

// String-based formatted output with size limit
int ksnprintf(char *buffer, size_t size, const char *fmt, ...);
int kvsnprintf(char *buffer, size_t size, const char *format, va_list args);

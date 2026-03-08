#pragma once
#include "vfs.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// File-based character/string output
void fputc(char c, fd_t file);
void fputs(const char *s, fd_t file);

// Standard output character/string output
void putc(char c);
void puts(const char *s);

// Debug output character/string output
void debugc(char c);
void debugs(const char *s);

// ============================================================================
// Formatted Output Functions
// ============================================================================

// File-based formatted output
void fprintf(fd_t file, const char *fmt, ...);
void vfprintf(fd_t file, const char *format, va_list args);

// Standard output formatted output
void printf(const char *fmt, ...);

// Debug formatted output
void debugf(const char *fmt, ...);

void debugf_and_printf(const char *fmt, ...);

// String-based formatted output
int sprintf(char *buffer, const char *fmt, ...);
int vsprintf(char *buffer, const char *format, va_list args);

// String-based formatted output with size limit
int snprintf(char *buffer, size_t size, const char *fmt, ...);
int vsnprintf(char *buffer, size_t size, const char *format, va_list args);

#pragma once
#include "fs/fd.h"
#include "fs/vfs.h"
#include "klib/stdarg.h"
#include "klib/stdint.h"

void kfputc(char c, fd_t file);
void kfputs(const char *s, fd_t file);
void kputc(char c);
void kputs(const char *s);
void kfprintf(fd_t file, const char *fmt, ...);
void kvfprintf(fd_t file, const char *format, va_list args);
void kprintf(const char *fmt, ...);
int ksprintf(char *buffer, const char *fmt, ...);
int kvsprintf(char *buffer, const char *format, va_list args);
int ksnprintf(char *buffer, size_t size, const char *fmt, ...);
int kvsnprintf(char *buffer, size_t size, const char *format, va_list args);

// Debug to debug stream
void kdebugc(char c);
void kdebugs(const char *s);
void kdebugf(const char *fmt, ...);
void kdebugf_and_printf(const char *fmt, ...);

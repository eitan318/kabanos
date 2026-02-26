// STDIO for custom libc
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef long fpos_t;

// Instead of bits/alltypes, use the compiler's versions
#include <stdarg.h> // Gives you va_list for printf
#include <stddef.h> // Gives you size_t and NULL

#undef EOF
#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Minimal FILE structure (Internal VFS handles the rest)
typedef struct _IO_FILE FILE;

extern FILE *const stdin;
extern FILE *const stdout;
extern FILE *const stderr;

// File Operations (Implement these later when VFS is ready)
FILE *fopen(const char *__restrict filename, const char *__restrict mode);
int fclose(FILE *stream);

int remove(const char *);
int rename(const char *, const char *);

int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
void clearerr(FILE *);

int fseek(FILE *, long, int);

int fgetpos(FILE *__restrict, fpos_t *__restrict);
int fsetpos(FILE *, const fpos_t *);

size_t fread(void *__restrict, size_t, size_t, FILE *__restrict);
size_t fwrite(const void *__restrict, size_t, size_t, FILE *__restrict);

int fgetc(FILE *);
int getc(FILE *);
int getchar(void);
int ungetc(int, FILE *);

int fputc(int, FILE *);
int putc(int, FILE *);
int putchar(int);

char *fgets(char *__restrict, int, FILE *__restrict);

int fputs(const char *__restrict, FILE *__restrict);
int puts(const char *);

int printf(const char *__restrict, ...);
int fprintf(FILE *__restrict, const char *__restrict, ...);
int sprintf(char *__restrict, const char *__restrict, ...);
int snprintf(char *__restrict, size_t, const char *__restrict, ...);

int vprintf(const char *__restrict, va_list);
int vfprintf(FILE *__restrict, const char *__restrict, va_list);
int vsprintf(char *__restrict, const char *__restrict, va_list);
int vsnprintf(char *__restrict, size_t, const char *__restrict, va_list);

int scanf(const char *__restrict, ...);
int fscanf(FILE *__restrict, const char *__restrict, ...);
int sscanf(const char *__restrict, const char *__restrict, ...);
int vscanf(const char *__restrict, va_list);
int vfscanf(FILE *__restrict, const char *__restrict, va_list);
int vsscanf(const char *__restrict, const char *__restrict, va_list);

void perror(const char *);

#ifdef __cplusplus
}
#endif

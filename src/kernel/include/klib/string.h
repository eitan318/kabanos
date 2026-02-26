#pragma once
#include <stdbool.h>
#include <stdint.h>

int atoi(const char *str);
void trim_newline(char *str);
void itoa(unsigned value, char *str);
const char *strchr(const char *str, char chr);
const char *strrchr(const char *str, char chr);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, unsigned n);
unsigned strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, unsigned n);
bool starts_with(const char *str, const char *prefix);
char *strtok(char *str, const char *delim);

char *strdup(const char *s);
char *strtok_r(char *str, const char *delim, char **saveptr);

void *memcpy(void *dst, const void *src, uint32_t num);
void *memset(void *ptr, int value, uint32_t num);
int memcmp(const void *ptr1, const void *ptr2, uint32_t num);

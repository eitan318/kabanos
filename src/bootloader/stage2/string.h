#pragma once
#include <stdbool.h>

void trim_newline(char *str);
void itoa(unsigned value, char *str);
const char *strchr(const char *str, char chr);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, unsigned n);
unsigned strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, unsigned n);
bool starts_with(const char *str, const char *prefix);
char *strtok(char *str, const char *delim);

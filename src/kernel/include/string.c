#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

const char *strrchr(const char *str, char chr) {
  if (str == NULL)
    return NULL;

  const char *last = NULL;

  // Scan through entire string, remembering last occurrence
  while (*str) {
    if (*str == chr)
      last = str;
    ++str;
  }

  return last;
}

const char *strchr(const char *str, char chr) {
  if (str == NULL)
    return NULL;

  while (*str) {
    if (*str == chr)
      return str;

    ++str;
  }

  return NULL;
}

char *strcpy(char *dst, const char *src) {
  char *origDst = dst;

  if (dst == NULL)
    return NULL;

  if (src == NULL) {
    *dst = '\0';
    return dst;
  }

  while (*src) {
    *dst = *src;
    ++src;
    ++dst;
  }

  *dst = '\0';
  return origDst;
}

char *strncpy(char *dst, const char *src, unsigned n) {
  char *origDst = dst;

  if (!dst)
    return NULL;

  if (!src) {
    while (n--)
      *dst++ = '\0';
    return origDst;
  }

  while (n && *src) {
    *dst++ = *src++;
    n--;
  }

  // pad remaining bytes with '\0'
  while (n--) {
    *dst++ = '\0';
  }

  return origDst;
}

unsigned strlen(const char *str) {
  unsigned len = 0;
  while (*str) {
    ++len;
    ++str;
  }

  return len;
}

int strcmp(const char *a, const char *b) {
  while (*a && (*a == *b)) {
    a++;
    b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, unsigned n) {
  while (n-- && *a && (*a == *b)) {
    a++;
    b++;
  }
  if (n == (unsigned)-1)
    return 0; // matched all
  return (unsigned char)*a - (unsigned char)*b;
}

bool starts_with(const char *str, const char *prefix) {
  while (*prefix) {
    if (*str++ != *prefix++)
      return false;
  }
  return true;
}

char *strtok(char *str, const char *delim) {
  static char *saved;
  if (str)
    saved = str;

  if (!saved)
    return NULL;

  // Skip leading delimiters
  while (*saved && strchr(delim, *saved))
    saved++;

  if (!*saved)
    return NULL;

  // Mark start of token
  char *token_start = saved;

  // Find end of token
  while (*saved && !strchr(delim, *saved))
    saved++;

  if (*saved) {
    *saved = '\0';
    saved++;
  } else {
    saved = NULL;
  }

  return token_start;
}

void itoa(unsigned value, char *str) {
  char tmp[16];
  int i = 0;

  if (value == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  while (value > 0) {
    tmp[i++] = '0' + (value % 10);
    value /= 10;
  }

  // reverse into output
  int j = 0;
  while (i > 0) {
    str[j++] = tmp[--i];
  }
  str[j] = '\0';
}

// Trim newline and carriage return from end of string
void trim_newline(char *str) {
  if (!str)
    return;

  char *p = str;
  // Find end of string or first newline/CR
  while (*p && *p != '\n' && *p != '\r')
    p++;

  // Null-terminate at newline position
  *p = '\0';
}

// Helper to convert integer to string
int atoi(const char *str) {
  int result = 0;
  int sign = 1;

  // Skip whitespace
  while (*str == ' ' || *str == '\t')
    str++;

  // Handle sign
  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }

  // Convert digits
  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }

  return sign * result;
}

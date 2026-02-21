#include "stdio.h"
#include "stdio_internal.h"

// ACTUAL DEFINITIONS
const char *sys_errlist[] = {
    "Success",
    "Operation not permitted",
    "No such file or directory",
    "Interrupted system call",
    "I/O error",
    "Exec format error",
};

int sys_nerr = 6;

int errno = 0;

void perror(const char *s) {
  const char *err_str;

  // 1. Get the string for the current error
  if (errno >= 0 && errno < sys_nerr) {
    err_str = sys_errlist[errno];
  } else {
    err_str = "Unknown error";
  }

  // 2. Print: "custom message: error description\n"
  if (s && *s) {
    fprintf(stderr, "%s: %s\n", s, err_str);
  } else {
    fprintf(stderr, "%s\n", err_str);
  }
}

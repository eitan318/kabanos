#include "cmdline.h"
#include "klib/stddef.h"
#include "klib/string.h"
#include "mm/kmalloc.h"

const char *cmdline_get_arg(const char *cmdline, const char *key,
                            size_t *val_len) {
  if (!cmdline || !key || !val_len) {
    return NULL;
  }

  const char *p = cmdline;
  size_t key_len = strlen(key);

  while (*p) {
    // If we are at the start of the tocken
    if ((p == cmdline || *(p - 1) == ' ') && strncmp(p, key, key_len) == 0 &&
        p[key_len] == '=') {
      const char *val_start = p + key_len + 1;
      *val_len = strcspn(val_start, " ");
      return val_start;
    }
    p++;
  }

  *val_len = 0;
  return NULL;
}

char *cmdline_get_arg_copy(const char *cmdline, const char *key) {
  size_t len;
  // Reuse the logic above to find the pointer and length
  const char *ptr = cmdline_get_arg(cmdline, key, &len);

  if (!ptr) {
    return NULL;
  }

  // Allocate len + 1 for the null terminator
  char *copy = (char *)kmalloc(len + 1);
  if (!copy) {
    return NULL;
  }

  // Use memcpy because we already know the exact length
  memcpy(copy, ptr, len);
  copy[len] = '\0';

  return copy;
}

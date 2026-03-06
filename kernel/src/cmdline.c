#include "cmdline.h"
#include "klib/string.h"

const char *cmdline_get_arg(const char *cmdline, const char *key) {
  if (!cmdline || !key)
    return NULL;

  const char *p = cmdline;
  size_t key_len = strlen(key);

  while (*p) {
    // Check if the current position matches the key
    // and is either at the start or preceded by a space
    if ((p == cmdline || *(p - 1) == ' ') && strncmp(p, key, key_len) == 0) {

      // Ensure the next character is '=' to confirm it's a key-value pair
      if (p[key_len] == '=') {
        return p + key_len + 1; // Return pointer to the start of the value
      }

      // If it matches exactly and followed by space or null, it's a flag
      if (p[key_len] == ' ' || p[key_len] == '\0') {
        return p; // Return the start of the flag itself
      }
    }
    p++;
  }

  return NULL;
}

void cmdline_copy_value(char *dest, const char *src, size_t max_len) {
  size_t i = 0;
  while (src[i] && src[i] != ' ' && i < max_len - 1) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

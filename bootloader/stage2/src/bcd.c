#include "bcd.h"
#include "boot/bootparams.h"
#include "s2lib/string.h"

// Construct kernel command line from BCD
void bcd_cmdline_construct(const char *bcd_cmdline, const int bcd_cmdline_size,
                           char *out) {
  strncpy(out, bcd_cmdline, bcd_cmdline_size - 1);
  out[bcd_cmdline_size - 1] = '\0';
}

// Parse boot configuration file
void bcd_parse_into(char *boot_config, BCD *out) {
  // Initialize structure
  out->kernel = NULL;
  out->initrd = NULL;
  out->module_count = 0;

  // Parse line by line (newline-delimited)
  char *line = strtok(boot_config, "\n\r");

  while (line) {
    // Trim any trailing whitespace
    trim_newline(line);

    // Skip empty lines and comments
    if (*line == '\0' || *line == '#') {
      line = strtok(NULL, "\n\r");
      continue;
    }

    // Parse key=value pairs
    if (starts_with(line, "kernel=")) {
      out->kernel = line + 7;
    } else if (starts_with(line, "initrd=")) {
      out->initrd = line + 7;
    } else if (starts_with(line, "cmdline=")) {
      out->cmdline = line + 8;
    } else if (starts_with(line, "module=")) {
      if (out->module_count < MAX_MODULES) {
        out->modules_paths[out->module_count] = line + 7;
        out->module_count++;
      }
    }

    line = strtok(NULL, "\n\r");
  }
}

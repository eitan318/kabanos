#include "cmdline.h"
#include "s2lib/string.h"

// Parse video mode string
VideoMode str_to_video(char *str) {
  VideoMode mode = {0};
  mode.mode_name = str;

  // Check for text mode
  if (!strcmp(str, "text")) {
    mode.type = 0;
    return mode;
  }

  // Check for named modes
  if (!strcmp(str, "vesa") || !strcmp(str, "vga")) {
    mode.type = 1;
    mode.width = 1024;
    mode.height = 768;
    mode.bpp = 32;
    return mode;
  }

  // Try to parse WIDTHxHEIGHTxBPP format
  const char *x1 = strchr(str, 'x');
  if (x1) {
    mode.type = 1;
    mode.width = atoi(str);
    const char *x2 = strchr(x1 + 1, 'x');
    if (x2) {
      mode.height = atoi(x1 + 1);
      mode.bpp = atoi(x2 + 1);
    } else {
      mode.height = atoi(x1 + 1);
      mode.bpp = 32; // default
    }
  }

  return mode;
}

// Convert video mode to string
static void video_to_str(VideoMode *v, char *out) {
  if (v->type == 0) {
    strcpy(out, "text");
    return;
  }

  // Graphics mode: WIDTHxHEIGHTxBPP
  char *p = out;
  char buf[16];

  // width
  itoa(v->width, buf);
  strcpy(p, buf);
  p += strlen(buf);
  *p++ = 'x';

  // height
  itoa(v->height, buf);
  strcpy(p, buf);
  p += strlen(buf);
  *p++ = 'x';

  // bpp
  itoa(v->bpp, buf);
  strcpy(p, buf);
  p += strlen(buf);
  *p = '\0';
}

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
  out->video = (VideoMode){0};

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
    } else if (starts_with(line, "video=")) {
      out->video = str_to_video(line + 6);
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

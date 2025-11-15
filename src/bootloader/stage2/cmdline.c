#include "cmdline.h"
#include "fat.h"
#include "string.h"

#define INITRD_ADDR 0x200000

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
void bcd_cmdline_construct(BCD *bcd, char *cmdline) {
  char *p = cmdline;

  // root=
  if (bcd->root_device) {
    strcpy(p, "root=");
    p += 5; // strlen("root=")
    strcpy(p, bcd->root_device);
    p += strlen(bcd->root_device);
    *p++ = ' ';
  }

  // debug
  if (bcd->debug_enabled) {
    strcpy(p, "debug ");
    p += 6; // strlen("debug ")
  }

  // video
  if (bcd->video.width || bcd->video.height || bcd->video.bpp ||
      bcd->video.type) {
    strcpy(p, "video=");
    p += 6; // strlen("video=")
    char buf[32];
    video_to_str(&bcd->video, buf);
    strcpy(p, buf);
    p += strlen(buf);
    *p++ = ' ';
  }

  // Terminate - remove trailing space
  if (p != cmdline && p[-1] == ' ')
    p[-1] = '\0';
  else
    *p = '\0';
}

// Parse boot configuration file
void bcd_parse_into(char *boot_config, BCD *out) {
  // Initialize structure
  out->root_device = NULL;
  out->kernel = NULL;
  out->initrd = NULL;
  out->module_count = 0;
  out->debug_enabled = 0;
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
    } else if (starts_with(line, "root=")) {
      out->root_device = line + 5;
    } else if (starts_with(line, "video=")) {
      out->video = str_to_video(line + 6);
    } else if (starts_with(line, "module=")) {
      if (out->module_count < MAX_MODULES) {
        out->modules[out->module_count].path = line + 7;
        out->module_count++;
      }
    } else if (!strcmp(line, "debug")) {
      out->debug_enabled = 1;
    }

    line = strtok(NULL, "\n\r");
  }
}

// Load initrd into memory
void load_initrd(char *initrd_path) {
  if (!initrd_path)
    return;

  int size = fat_read_file(initrd_path, (void *)INITRD_ADDR);
  if (size < 0) {
    // Handle error - could use debugf here if available
  }
}

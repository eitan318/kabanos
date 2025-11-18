#include "initrd.h"
#include "include/stdio.h"
#include "include/string.h"
#include <stddef.h>

// Simple TAR header for initrd
typedef struct {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char padding[12];
} TarHeader;

static void *g_initrd_start = NULL;
static uint32_t g_initrd_size = 0;

// Parse octal string from TAR header
static uint32_t tar_parse_octal(const char *str, size_t len) {
  uint32_t val = 0;
  for (size_t i = 0; i < len && str[i] != '\0' && str[i] != ' '; i++) {
    val = val * 8 + (str[i] - '0');
  }
  return val;
}

// Initialize initrd
void initrd_init(void *initrd_start, uint32_t initrd_size) {

  g_initrd_start = initrd_start;
  g_initrd_size = initrd_size;

  debugf("Initrd initialized at 0x%p, size: %u bytes\n", initrd_start,
         initrd_size);
}

// Find a file in the initrd
void *initrd_find_file(const char *filename, uint32_t *size_out) {
  if (!g_initrd_start) {
    return NULL;
  }

  uint8_t *ptr = (uint8_t *)g_initrd_start;
  uint8_t *end = ptr + g_initrd_size;

  while (ptr < end) {
    TarHeader *header = (TarHeader *)ptr;

    // Check for end of archive (all zeros)
    if (header->name[0] == '\0') {
      break;
    }

    // Verify TAR magic
    if (strncmp(header->magic, "ustar", 5) != 0) {
      debugf("Invalid TAR magic at 0x%p\n", ptr);
      break;
    }

    uint32_t file_size = tar_parse_octal(header->size, sizeof(header->size));
    void *file_data = ptr + 512; // TAR header is always 512 bytes

    // Check if this is the file we're looking for
    if (strcmp(header->name, filename) == 0) {
      if (size_out) {
        *size_out = file_size;
      }
      debugf("Found file '%s' in initrd, size: %u bytes\n", filename,
             file_size);
      return file_data;
    }

    // Move to next file (align to 512-byte boundary)
    uint32_t blocks = (file_size + 511) / 512;
    ptr += 512 + (blocks * 512);
  }

  return NULL;
}

// List all files in initrd
void initrd_list_files(void) {
  if (!g_initrd_start) {
    debugf("Initrd not initialized\n");
    return;
  }

  debugf("Files in initrd:\n");

  uint8_t *ptr = (uint8_t *)g_initrd_start;
  uint8_t *end = ptr + g_initrd_size;

  while (ptr < end) {
    TarHeader *header = (TarHeader *)ptr;

    if (header->name[0] == '\0') {
      break;
    }

    if (strncmp(header->magic, "ustar", 5) != 0) {
      break;
    }

    uint32_t file_size = tar_parse_octal(header->size, sizeof(header->size));
    debugf("  %s (%u bytes)\n", header->name, file_size);

    uint32_t blocks = (file_size + 511) / 512;
    ptr += 512 + (blocks * 512);
  }
}

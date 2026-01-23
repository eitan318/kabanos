#include "fat.h"
#include "drivers/ata.h"
#include "memory_management/kmalloc.h"
#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE -1

enum FAT_Attributes {
  FAT_ATTRIBUTE_READ_ONLY = 0x01,
  FAT_ATTRIBUTE_HIDDEN = 0x02,
  FAT_ATTRIBUTE_SYSTEM = 0x04,
  FAT_ATTRIBUTE_VOLUME_ID = 0x08,
  FAT_ATTRIBUTE_DIRECTORY = 0x10,
  FAT_ATTRIBUTE_ARCHIVE = 0x20,
  FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN |
                      FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

typedef struct __attribute__((packed)) {
  uint8_t boot_jump_instruction[3];
  uint8_t oem_identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t dir_entry_count;
  uint16_t total_sectors;
  uint8_t media_descriptor_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t large_sector_count;
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t signature;
  uint32_t volume_id;
  uint8_t volume_label[11];
  uint8_t system_id[8];
} FAT_BootSector;

typedef struct {
  uint8_t *buffer;
  FAT_File public;
  bool opened;
  uint32_t first_cluster;
  uint32_t current_cluster;
  uint32_t current_sector_in_cluster;
} FAT_FileData;

typedef struct {
  FAT_BootSector boot_sector;
  FAT_FileData root_directory;
  FAT_FileData opened_files[MAX_FILE_HANDLES];
  uint8_t *fat_table;
  uint32_t data_section_lba;
  uint32_t partition_lba;
  bool initialized;
} FAT_Data;

static FAT_Data g_fat_data = {0};

// Helper to convert toupper
static char toupper(char c) {
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';
  return c;
}

// Helper min
static uint32_t min_uint32(uint32_t a, uint32_t b) { return a < b ? a : b; }

static bool fat_read_boot_sector(void) {
  uint8_t *boot_sector_buf = kmalloc(SECTOR_SIZE);
  if (!boot_sector_buf) {
    return false;
  }

  ata_read_sector(g_fat_data.partition_lba, 1, boot_sector_buf);
  memcpy(&g_fat_data.boot_sector, boot_sector_buf, sizeof(FAT_BootSector));
  kfree(boot_sector_buf);

  return true;
}

static bool fat_read_fat_table(void) {
  uint32_t fat_size = g_fat_data.boot_sector.bytes_per_sector *
                      g_fat_data.boot_sector.sectors_per_fat;

  g_fat_data.fat_table = kmalloc(fat_size);
  if (!g_fat_data.fat_table) {
    return false;
  }

  uint32_t fat_lba =
      g_fat_data.partition_lba + g_fat_data.boot_sector.reserved_sectors;
  uint32_t sectors = g_fat_data.boot_sector.sectors_per_fat;

  // Read FAT table sector by sector
  for (uint32_t i = 0; i < sectors; i++) {
    ata_read_sector(fat_lba + i, 1, g_fat_data.fat_table + (i * SECTOR_SIZE));
  }

  return true;
}

bool fat_initialize(uint32_t partition_lba) {
  memset(&g_fat_data, 0, sizeof(FAT_Data));
  g_fat_data.partition_lba = partition_lba;

  if (!fat_read_boot_sector()) {
    return false;
  }

  if (!fat_read_fat_table()) {
    return false;
  }

  uint32_t root_dir_lba =
      g_fat_data.partition_lba + g_fat_data.boot_sector.reserved_sectors +
      g_fat_data.boot_sector.sectors_per_fat * g_fat_data.boot_sector.fat_count;

  uint32_t root_dir_size =
      sizeof(FAT_DirectoryEntry) * g_fat_data.boot_sector.dir_entry_count;

  g_fat_data.root_directory.buffer = kmalloc(SECTOR_SIZE);
  if (!g_fat_data.root_directory.buffer) {
    kfree(g_fat_data.fat_table);
    return false;
  }

  g_fat_data.root_directory.public.handle = ROOT_DIRECTORY_HANDLE;
  g_fat_data.root_directory.public.is_directory = true;
  g_fat_data.root_directory.public.position = 0;
  g_fat_data.root_directory.public.size = root_dir_size;
  g_fat_data.root_directory.opened = true;
  g_fat_data.root_directory.first_cluster = root_dir_lba;
  g_fat_data.root_directory.current_cluster = root_dir_lba;
  g_fat_data.root_directory.current_sector_in_cluster = 0;

  ata_read_sector(root_dir_lba, 1, g_fat_data.root_directory.buffer);

  uint32_t root_dir_sectors =
      (root_dir_size + g_fat_data.boot_sector.bytes_per_sector - 1) /
      g_fat_data.boot_sector.bytes_per_sector;

  g_fat_data.data_section_lba = root_dir_lba + root_dir_sectors;

  for (int i = 0; i < MAX_FILE_HANDLES; i++) {
    g_fat_data.opened_files[i].opened = false;
    g_fat_data.opened_files[i].buffer = NULL;
  }

  g_fat_data.initialized = true;
  return true;
}

static uint32_t fat_cluster_to_lba(uint32_t cluster) {
  return g_fat_data.data_section_lba +
         (cluster - 2) * g_fat_data.boot_sector.sectors_per_cluster;
}

static FAT_File *fat_open_entry(FAT_DirectoryEntry *entry) {
  int handle = -1;
  for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
    if (!g_fat_data.opened_files[i].opened) {
      handle = i;
    }
  }

  if (handle < 0) {
    return NULL;
  }

  FAT_FileData *fd = &g_fat_data.opened_files[handle];

  fd->buffer = kmalloc(SECTOR_SIZE);
  if (!fd->buffer) {
    return NULL;
  }

  fd->public.handle = handle;
  fd->public.is_directory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
  fd->public.position = 0;
  fd->public.size = entry->size;

  fd->first_cluster =
      entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16);

  fd->current_cluster = fd->first_cluster;
  fd->current_sector_in_cluster = 0;

  ata_read_sector(fat_cluster_to_lba(fd->current_cluster), 1, fd->buffer);

  fd->opened = true;
  return &fd->public;
}

static uint32_t fat_next_cluster(uint32_t current_cluster) {
  uint32_t fat_index = current_cluster * 3 / 2;

  if (current_cluster % 2 == 0)
    return (*(uint16_t *)(g_fat_data.fat_table + fat_index)) & 0x0FFF;
  else
    return (*(uint16_t *)(g_fat_data.fat_table + fat_index)) >> 4;
}

uint32_t fat_read(FAT_File *file, uint32_t byte_count, void *out) {
  FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                         ? &g_fat_data.root_directory
                         : &g_fat_data.opened_files[file->handle];

  uint8_t *out8 = (uint8_t *)out;

  // Limit read to file size (except for directories with unknown size)
  if (!fd->public.is_directory ||
      (fd->public.is_directory && fd->public.size != 0)) {
    byte_count = min_uint32(byte_count, fd->public.size - fd->public.position);
  }

  while (byte_count > 0) {
    uint32_t left_in_buffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
    uint32_t take = min_uint32(byte_count, left_in_buffer);

    memcpy(out8, fd->buffer + (fd->public.position % SECTOR_SIZE), take);

    out8 += take;
    fd->public.position += take;
    byte_count -= take;

    // Need to read next sector?
    if (left_in_buffer == take) {
      if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
        // Root directory is at a fixed location, not cluster-based
        ++fd->current_sector_in_cluster; // Use as sector counter

        // Calculate next root directory sector
        uint32_t root_sector =
            fd->first_cluster + fd->current_sector_in_cluster;

        // Check if we've read all root directory sectors
        uint32_t root_size =
            sizeof(FAT_DirectoryEntry) * g_fat_data.boot_sector.dir_entry_count;
        uint32_t root_sectors = (root_size + SECTOR_SIZE - 1) / SECTOR_SIZE;

        if (fd->current_sector_in_cluster >= root_sectors) {
          // End of root directory
          break;
        }

        // Read next root directory sector
        ata_read_sector(root_sector, 1, fd->buffer);

      } else {
        // Regular file - use cluster chain
        if (++fd->current_sector_in_cluster >=
            g_fat_data.boot_sector.sectors_per_cluster) {
          fd->current_sector_in_cluster = 0;
          fd->current_cluster = fat_next_cluster(fd->current_cluster);
        }

        // Check for end of cluster chain
        if (fd->current_cluster >= 0xFF8) {
          fd->public.size = fd->public.position;
          break;
        }

        // Read next sector from cluster
        ata_read_sector(fat_cluster_to_lba(fd->current_cluster) +
                            fd->current_sector_in_cluster,
                        1, fd->buffer);
      }
    }
  }

  return out8 - (uint8_t *)out;
}

bool fat_read_entry(FAT_File *file, FAT_DirectoryEntry *entry) {
  return fat_read(file, sizeof(FAT_DirectoryEntry), entry) ==
         sizeof(FAT_DirectoryEntry);
}

void fat_close(FAT_File *file) {
  if (file->handle == ROOT_DIRECTORY_HANDLE) {
    file->position = 0;
    g_fat_data.root_directory.current_cluster =
        g_fat_data.root_directory.first_cluster;
  } else {
    if (g_fat_data.opened_files[file->handle].buffer) {
      kfree(g_fat_data.opened_files[file->handle].buffer);
      g_fat_data.opened_files[file->handle].buffer = NULL;
    }
    g_fat_data.opened_files[file->handle].opened = false;
  }
}

static bool fat_find_file(FAT_File *file, const char *name,
                          FAT_DirectoryEntry *out) {
  char fat_name[12];
  FAT_DirectoryEntry entry;

  memset(fat_name, ' ', sizeof(fat_name));
  fat_name[11] = '\0';

  const char *ext = strchr(name, '.');
  if (ext == NULL)
    ext = name + 11;

  for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
    fat_name[i] = toupper(name[i]);

  if (ext != name + 11) {
    for (int i = 0; i < 3 && ext[i + 1]; i++)
      fat_name[i + 8] = toupper(ext[i + 1]);
  }

  while (fat_read_entry(file, &entry)) {
    if (memcmp(fat_name, entry.name, 11) == 0) {
      *out = entry;
      return true;
    }
  }

  return false;
}

FAT_File *fat_open(const char *path) {
  if (!g_fat_data.initialized) {
    return NULL;
  }

  char name[MAX_PATH_SIZE];

  if (path[0] == '/')
    path++;

  FAT_File *current = &g_fat_data.root_directory.public;

  while (*path) {
    bool is_last = false;
    const char *delim = strchr(path, '/');

    if (delim != NULL) {
      memcpy(name, path, delim - path);
      name[delim - path] = '\0';
      path = delim + 1;
    } else {
      unsigned len = strlen(path);
      memcpy(name, path, len);
      name[len] = '\0';
      path += len;
      is_last = true;
    }

    FAT_DirectoryEntry entry;

    if (fat_find_file(current, name, &entry)) {
      fat_close(current);

      if (!is_last && (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0) {
        return NULL;
      }

      current = fat_open_entry(&entry);
    } else {
      fat_close(current);
      return NULL;
    }
  }

  return current;
}

int fat_read_file(const char *path, void **buffer, uint32_t *size) {
  if (!g_fat_data.initialized) {
    return -1;
  }

  debugf("FAT: Opening file '%s'...\n", path);
  FAT_File *file = fat_open(path);

  if (!file) {
    debugf("FAT: Could not open: %s\n", path);
    return -2;
  }

  debugf("FAT: File size: %u bytes\n", file->size);

  *buffer = kmalloc(file->size);
  if (!*buffer) {
    fat_close(file);
    return -3;
  }

  uint32_t read = fat_read(file, file->size, *buffer);
  *size = file->size;

  fat_close(file);

  if (read != file->size) {
    kfree(*buffer);
    *buffer = NULL;
    return -4;
  }

  debugf("FAT: File loaded successfully (%u bytes)\n", read);
  return 0;
}

void fat_list_root_directory(void) {
  if (!g_fat_data.initialized) {
    return;
  }

  // Read root directory manually, sector by sector
  uint32_t root_dir_size =
      sizeof(FAT_DirectoryEntry) * g_fat_data.boot_sector.dir_entry_count;
  uint32_t root_sectors = (root_dir_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
  uint32_t root_lba = g_fat_data.root_directory.first_cluster;

  uint8_t *sector_buf = kmalloc(SECTOR_SIZE);
  if (!sector_buf) {
    return;
  }

  int total_count = 0;
  bool found_end = false;

  for (uint32_t sector = 0; sector < root_sectors && !found_end; sector++) {
    ata_read_sector(root_lba + sector, 1, sector_buf);

    FAT_DirectoryEntry *entries = (FAT_DirectoryEntry *)sector_buf;
    uint32_t entries_per_sector = SECTOR_SIZE / sizeof(FAT_DirectoryEntry);

    for (uint32_t i = 0; i < entries_per_sector && !found_end; i++) {
      FAT_DirectoryEntry *entry = &entries[i];

      // Check for end of directory
      if (entry->name[0] == 0x00) {
        found_end = true;
        break;
      }

      // Skip deleted entries
      if (entry->name[0] == 0xE5) {
        continue;
      }

      // Skip volume labels and LFN entries
      if (entry->attributes == FAT_ATTRIBUTE_VOLUME_ID ||
          entry->attributes == FAT_ATTRIBUTE_LFN) {
        continue;
      }

      // Extract filename
      char filename[13];
      int pos = 0;

      // Copy name part (8 chars)
      for (int j = 0; j < 8 && entry->name[j] != ' '; j++) {
        // Ensure printable character
        if (entry->name[j] >= 32 && entry->name[j] < 127) {
          filename[pos++] = entry->name[j];
        }
      }

      // Add extension if present
      if (entry->name[8] != ' ') {
        filename[pos++] = '.';
        for (int j = 8; j < 11 && entry->name[j] != ' '; j++) {
          // Ensure printable character
          if (entry->name[j] >= 32 && entry->name[j] < 127) {
            filename[pos++] = entry->name[j];
          }
        }
      }

      filename[pos] = '\0';

      // Only count valid files
      if (pos > 0) {
        total_count++;
      }
    }
  }

  kfree(sector_buf);
}

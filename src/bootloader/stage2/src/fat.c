#include "fat.h"
#include "ctype.h"
#include "memory.h"
#include "s2lib/stdio.h"
#include "string.h"
#include "utils/math.h"
#include <stdbool.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE -1

#define MEMORY_FAT_ADDR ((void *)0x20000)
#define MEMORY_FAT_SIZE 0x00010000

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
  uint8_t buffer[SECTOR_SIZE];
  FAT_File public;
  bool opened;
  uint32_t first_cluster;
  uint32_t current_cluster;
  uint32_t current_sector_in_cluster;
} FAT_FileData;

typedef struct {
  union {
    FAT_BootSector boot_sector;
    uint8_t boot_sector_bytes[SECTOR_SIZE];
  } bs;

  FAT_FileData root_directory;
  FAT_FileData opened_files[MAX_FILE_HANDLES];
} FAT_Data;

static FAT_Data *g_data;
static uint8_t *g_fat = NULL;
static uint32_t g_data_section_lba;
static Partition *g_disk = NULL;

bool fat_read_boot_sector(Partition *disk) {
  return Partition_read_sectors(disk, 0, 1, g_data->bs.boot_sector_bytes);
}

bool fat_read_fat(Partition *disk) {
  return Partition_read_sectors(disk, g_data->bs.boot_sector.reserved_sectors,
                                g_data->bs.boot_sector.sectors_per_fat, g_fat);
}

bool fat_initialize(Partition *disk) {
  g_data = (FAT_Data *)MEMORY_FAT_ADDR;
  g_disk = disk;

  if (!fat_read_boot_sector(disk)) {
    debugf("FAT: read boot sector failed\n");
    return false;
  }

  // debugf("FAT: Boot sector loaded\n");
  // debugf("  Bytes/Sector: %u\n", g_data->bs.boot_sector.bytes_per_sector);
  // debugf("  Sectors/Cluster: %u\n",
  // g_data->bs.boot_sector.sectors_per_cluster); debugf("  FAT copies: %u\n",
  // g_data->bs.boot_sector.fat_count); debugf("  Root entries: %u\n",
  // g_data->bs.boot_sector.dir_entry_count);

  g_fat = (uint8_t *)g_data + sizeof(FAT_Data);
  uint32_t fat_size = g_data->bs.boot_sector.bytes_per_sector *
                      g_data->bs.boot_sector.sectors_per_fat;

  if (sizeof(FAT_Data) + fat_size >= MEMORY_FAT_SIZE) {
    debugf("FAT: not enough memory! required=%lu available=%u\n",
           sizeof(FAT_Data) + fat_size, MEMORY_FAT_SIZE);
    return false;
  }

  if (!fat_read_fat(disk)) {
    debugf("FAT: read FAT failed\n");
    return false;
  }

  uint32_t root_dir_lba =
      g_data->bs.boot_sector.reserved_sectors +
      g_data->bs.boot_sector.sectors_per_fat * g_data->bs.boot_sector.fat_count;

  uint32_t root_dir_size =
      sizeof(FAT_DirectoryEntry) * g_data->bs.boot_sector.dir_entry_count;

  g_data->root_directory.public.handle = ROOT_DIRECTORY_HANDLE;
  g_data->root_directory.public.is_directory = true;
  g_data->root_directory.public.position = 0;
  g_data->root_directory.public.size = root_dir_size;
  g_data->root_directory.opened = true;
  g_data->root_directory.first_cluster = root_dir_lba;
  g_data->root_directory.current_cluster = root_dir_lba;
  g_data->root_directory.current_sector_in_cluster = 0;

  if (!Partition_read_sectors(disk, root_dir_lba, 1,
                              g_data->root_directory.buffer)) {
    debugf("FAT: read root directory failed\n");
    return false;
  }

  uint32_t root_dir_sectors =
      (root_dir_size + g_data->bs.boot_sector.bytes_per_sector - 1) /
      g_data->bs.boot_sector.bytes_per_sector;

  g_data_section_lba = root_dir_lba + root_dir_sectors;

  for (int i = 0; i < MAX_FILE_HANDLES; i++)
    g_data->opened_files[i].opened = false;

  debugf("FAT: Filesystem initialized successfully\n");
  return true;
}

uint32_t fat_cluster_to_lba(uint32_t cluster) {
  return g_data_section_lba +
         (cluster - 2) * g_data->bs.boot_sector.sectors_per_cluster;
}

FAT_File *fat_open_entry(Partition *disk, FAT_DirectoryEntry *entry) {
  int handle = -1;
  for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    if (!g_data->opened_files[i].opened)
      handle = i;

  if (handle < 0) {
    debugf("FAT: out of file handles\n");
    return NULL;
  }

  FAT_FileData *fd = &g_data->opened_files[handle];

  fd->public.handle = handle;
  fd->public.is_directory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
  fd->public.position = 0;
  fd->public.size = entry->size;

  fd->first_cluster =
      entry->first_cluster_low + ((uint32_t)entry->first_cluster_high << 16);

  fd->current_cluster = fd->first_cluster;
  fd->current_sector_in_cluster = 0;

  if (!Partition_read_sectors(disk, fat_cluster_to_lba(fd->current_cluster), 1,
                              fd->buffer)) {
    debugf("FAT: open entry failed\n");
    return NULL;
  }

  fd->opened = true;
  return &fd->public;
}

uint32_t fat_next_cluster(uint32_t current_cluster) {
  uint32_t fat_index = current_cluster * 3 / 2;

  if (current_cluster % 2 == 0)
    return (*(uint16_t *)(g_fat + fat_index)) & 0x0FFF;
  else
    return (*(uint16_t *)(g_fat + fat_index)) >> 4;
}

uint32_t fat_read(Partition *disk, FAT_File *file, uint32_t byte_count,
                  void *out) {
  FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                         ? &g_data->root_directory
                         : &g_data->opened_files[file->handle];

  uint8_t *out8 = (uint8_t *)out;

  if (!fd->public.is_directory ||
      (fd->public.is_directory && fd->public.size != 0))
    byte_count = min_int(byte_count, fd->public.size - fd->public.position);

  while (byte_count > 0) {
    uint32_t left_in_buffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);

    uint32_t take = min_int(byte_count, left_in_buffer);

    memcpy(out8, fd->buffer + (fd->public.position % SECTOR_SIZE), take);

    out8 += take;
    fd->public.position += take;
    byte_count -= take;

    if (left_in_buffer == take) {
      if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
        ++fd->current_cluster;

        if (!Partition_read_sectors(disk, fd->current_cluster, 1, fd->buffer)) {
          debugf("FAT: read error!\n");
          break;
        }
      } else {
        if (++fd->current_sector_in_cluster >=
            g_data->bs.boot_sector.sectors_per_cluster) {
          fd->current_sector_in_cluster = 0;
          fd->current_cluster = fat_next_cluster(fd->current_cluster);
        }

        if (fd->current_cluster >= 0xFF8) {
          fd->public.size = fd->public.position;
          break;
        }

        if (!Partition_read_sectors(disk,
                                    fat_cluster_to_lba(fd->current_cluster) +
                                        fd->current_sector_in_cluster,
                                    1, fd->buffer)) {
          debugf("FAT: read error!\n");
          break;
        }
      }
    }
  }

  return out8 - (uint8_t *)out;
}

bool fat_read_entry(Partition *disk, FAT_File *file,
                    FAT_DirectoryEntry *entry) {
  return fat_read(disk, file, sizeof(FAT_DirectoryEntry), entry) ==
         sizeof(FAT_DirectoryEntry);
}

void fat_close(FAT_File *file) {
  if (file->handle == ROOT_DIRECTORY_HANDLE) {
    file->position = 0;
    g_data->root_directory.current_cluster =
        g_data->root_directory.first_cluster;
  } else {
    g_data->opened_files[file->handle].opened = false;
  }
}

bool fat_find_file(Partition *disk, FAT_File *file, const char *name,
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

  while (fat_read_entry(disk, file, &entry)) {
    if (memcmp(fat_name, entry.name, 11) == 0) {
      *out = entry;
      return true;
    }
  }

  return false;
}

FAT_File *fat_open(Partition *disk, const char *path) {
  char name[MAX_PATH_SIZE];

  if (path[0] == '/')
    path++;

  FAT_File *current = &g_data->root_directory.public;

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

    if (fat_find_file(disk, current, name, &entry)) {
      fat_close(current);

      if (!is_last && (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0) {
        debugf("FAT: %s not a directory\n", name);
        return NULL;
      }

      current = fat_open_entry(disk, &entry);
    } else {
      fat_close(current);
      debugf("FAT: %s not found\n", name);
      return NULL;
    }
  }

  return current;
}

int fat_read_file(const char *path, void *buffer) {
  if (!g_disk) {
    debugf("ERROR: FAT not initialized!\n");
    return -1;
  }

  FAT_File *file = fat_open(g_disk, path);

  if (!file) {
    debugf("ERROR: Could not open: %s\n", path);
    return -2;
  }

  uint32_t read = fat_read(g_disk, file, file->size, buffer);

  fat_close(file);

  if (read != file->size) {
    debugf("ERROR: Read %u bytes, expected %u\n", read, file->size);
    return -3;
  }

  debugf("FAT: File %s loaded successfully (%u bytes) to %p\n", path, read,
         buffer);
  return read;
}

void fat_shutdown(void) {
  if (g_fat_data.fat_table) {
    kfree(g_fat_data.fat_table);
    g_fat_data.fat_table = NULL;
  }
  if (g_fat_data.root_directory.buffer) {
    kfree(g_fat_data.root_directory.buffer);
    g_fat_data.root_directory.buffer = NULL;
  }
  g_fat_data.initialized = false;
}

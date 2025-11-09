#include "fat.h"
#include "ctype.h"
#include "memory.h"
#include "stdio.h"
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

typedef struct __attribute__((packed)) {
  uint8_t Name[11];
  uint8_t Attributes;
  uint8_t _Reserved;
  uint8_t CreatedTimeTenths;
  uint16_t CreatedTime;
  uint16_t CreatedDate;
  uint16_t AccessedDate;
  uint16_t FirstClusterHigh;
  uint16_t ModifiedTime;
  uint16_t ModifiedDate;
  uint16_t FirstClusterLow;
  uint32_t Size;
} FAT_DirectoryEntry;

typedef struct {
  int Handle;
  bool IsDirectory;
  uint32_t Position;
  uint32_t Size;
} FAT_File;

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
  uint8_t BootJumpInstruction[3];
  uint8_t OemIdentifier[8];
  uint16_t BytesPerSector;
  uint8_t SectorsPerCluster;
  uint16_t ReservedSectors;
  uint8_t FatCount;
  uint16_t DirEntryCount;
  uint16_t TotalSectors;
  uint8_t MediaDescriptorType;
  uint16_t SectorsPerFat;
  uint16_t SectorsPerTrack;
  uint16_t Heads;
  uint32_t HiddenSectors;
  uint32_t LargeSectorCount;
  uint8_t DriveNumber;
  uint8_t _Reserved;
  uint8_t Signature;
  uint32_t VolumeId;
  uint8_t VolumeLabel[11];
  uint8_t SystemId[8];
} FAT_BootSector;

typedef struct {
  uint8_t Buffer[SECTOR_SIZE];
  FAT_File Public;
  bool Opened;
  uint32_t FirstCluster;
  uint32_t CurrentCluster;
  uint32_t CurrentSectorInCluster;
} FAT_FileData;

typedef struct {
  union {
    FAT_BootSector BootSector;
    uint8_t BootSectorBytes[SECTOR_SIZE];
  } BS;

  FAT_FileData RootDirectory;
  FAT_FileData OpenedFiles[MAX_FILE_HANDLES];
} FAT_Data;

static FAT_Data *g_Data;
static uint8_t *g_Fat = NULL;
static uint32_t g_DataSectionLba;
static DiskParams *g_Disk = NULL;

bool FAT_ReadBootSector(DiskParams *disk, uint32_t lba) {
  return disk_read_sectors(disk, lba, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DiskParams *disk, uint32_t lba) {
  return disk_read_sectors(disk, lba + g_Data->BS.BootSector.ReservedSectors,
                           g_Data->BS.BootSector.SectorsPerFat, g_Fat);
}

bool FAT_Initialize(DiskParams *disk, uint32_t partition_lba) {
  debugf("FAT: Initializing filesystem...\n");

  // Use static memory allocation
  g_Data = (FAT_Data *)MEMORY_FAT_ADDR;
  g_Disk = disk;

  // Read boot sector from partition
  if (!FAT_ReadBootSector(disk, partition_lba)) {
    debugf("FAT: read boot sector failed\n");
    return false;
  }

  debugf("FAT: Boot sector loaded\n");
  debugf("  Bytes/Sector: %u\n", g_Data->BS.BootSector.BytesPerSector);
  debugf("  Sectors/Cluster: %u\n", g_Data->BS.BootSector.SectorsPerCluster);
  debugf("  FAT copies: %u\n", g_Data->BS.BootSector.FatCount);
  debugf("  Root entries: %u\n", g_Data->BS.BootSector.DirEntryCount);

  // Setup FAT pointer right after FAT_Data structure
  g_Fat = (uint8_t *)g_Data + sizeof(FAT_Data);
  uint32_t fatSize = g_Data->BS.BootSector.BytesPerSector *
                     g_Data->BS.BootSector.SectorsPerFat;

  if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE) {
    debugf("FAT: not enough memory to read FAT! Required %lu, only have %u\n",
           sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
    return false;
  }

  if (!FAT_ReadFat(disk, partition_lba)) {
    debugf("FAT: read FAT failed\n");
    return false;
  }

  debugf("FAT: FAT table loaded (%u bytes)\n", fatSize);

  // Open root directory
  uint32_t rootDirLba =
      partition_lba + g_Data->BS.BootSector.ReservedSectors +
      g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
  uint32_t rootDirSize =
      sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;

  g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
  g_Data->RootDirectory.Public.IsDirectory = true;
  g_Data->RootDirectory.Public.Position = 0;
  g_Data->RootDirectory.Public.Size = rootDirSize;
  g_Data->RootDirectory.Opened = true;
  g_Data->RootDirectory.FirstCluster = rootDirLba;
  g_Data->RootDirectory.CurrentCluster = rootDirLba;
  g_Data->RootDirectory.CurrentSectorInCluster = 0;

  if (!disk_read_sectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer)) {
    debugf("FAT: read root directory failed\n");
    return false;
  }

  // Calculate data section LBA
  uint32_t rootDirSectors =
      (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) /
      g_Data->BS.BootSector.BytesPerSector;
  g_DataSectionLba = rootDirLba + rootDirSectors;

  // Reset opened files
  for (int i = 0; i < MAX_FILE_HANDLES; i++)
    g_Data->OpenedFiles[i].Opened = false;

  debugf("FAT: Filesystem initialized successfully\n");
  return true;
}

uint32_t FAT_ClusterToLba(uint32_t cluster) {
  return g_DataSectionLba +
         (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

FAT_File *FAT_OpenEntry(DiskParams *disk, FAT_DirectoryEntry *entry) {
  // Find empty handle
  int handle = -1;
  for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
    if (!g_Data->OpenedFiles[i].Opened)
      handle = i;
  }

  if (handle < 0) {
    debugf("FAT: out of file handles\n");
    return NULL;
  }

  // Setup file data
  FAT_FileData *fd = &g_Data->OpenedFiles[handle];
  fd->Public.Handle = handle;
  fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
  fd->Public.Position = 0;
  fd->Public.Size = entry->Size;
  fd->FirstCluster =
      entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
  fd->CurrentCluster = fd->FirstCluster;
  fd->CurrentSectorInCluster = 0;

  if (!disk_read_sectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1,
                         fd->Buffer)) {
    debugf("FAT: open entry failed - read error cluster=%u lba=%u\n",
           fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
    return NULL;
  }

  fd->Opened = true;
  return &fd->Public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster) {
  uint32_t fatIndex = currentCluster * 3 / 2;

  if (currentCluster % 2 == 0)
    return (*(uint16_t *)(g_Fat + fatIndex)) & 0x0FFF;
  else
    return (*(uint16_t *)(g_Fat + fatIndex)) >> 4;
}

uint32_t FAT_Read(DiskParams *disk, FAT_File *file, uint32_t byteCount,
                  void *dataOut) {
  FAT_FileData *fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
                         ? &g_Data->RootDirectory
                         : &g_Data->OpenedFiles[file->Handle];

  uint8_t *u8DataOut = (uint8_t *)dataOut;

  if (!fd->Public.IsDirectory ||
      (fd->Public.IsDirectory && fd->Public.Size != 0))
    byteCount = min_int(byteCount, fd->Public.Size - fd->Public.Position);

  while (byteCount > 0) {
    uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
    uint32_t take = min_int(byteCount, leftInBuffer);

    memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
    u8DataOut += take;
    fd->Public.Position += take;
    byteCount -= take;

    if (leftInBuffer == take) {
      if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE) {
        ++fd->CurrentCluster;

        if (!disk_read_sectors(disk, fd->CurrentCluster, 1, fd->Buffer)) {
          debugf("FAT: read error!\n");
          break;
        }
      } else {
        if (++fd->CurrentSectorInCluster >=
            g_Data->BS.BootSector.SectorsPerCluster) {
          fd->CurrentSectorInCluster = 0;
          fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
        }

        if (fd->CurrentCluster >= 0xFF8) {
          fd->Public.Size = fd->Public.Position;
          break;
        }

        if (!disk_read_sectors(disk,
                               FAT_ClusterToLba(fd->CurrentCluster) +
                                   fd->CurrentSectorInCluster,
                               1, fd->Buffer)) {
          debugf("FAT: read error!\n");
          break;
        }
      }
    }
  }

  return u8DataOut - (uint8_t *)dataOut;
}

bool FAT_ReadEntry(DiskParams *disk, FAT_File *file,
                   FAT_DirectoryEntry *dirEntry) {
  return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) ==
         sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File *file) {
  if (file->Handle == ROOT_DIRECTORY_HANDLE) {
    file->Position = 0;
    g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
  } else {
    g_Data->OpenedFiles[file->Handle].Opened = false;
  }
}

bool FAT_FindFile(DiskParams *disk, FAT_File *file, const char *name,
                  FAT_DirectoryEntry *entryOut) {
  char fatName[12];
  FAT_DirectoryEntry entry;

  memset(fatName, ' ', sizeof(fatName));
  fatName[11] = '\0';

  const char *ext = strchr(name, '.');
  if (ext == NULL)
    ext = name + 11;

  for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
    fatName[i] = toupper(name[i]);

  if (ext != name + 11) {
    for (int i = 0; i < 3 && ext[i + 1]; i++)
      fatName[i + 8] = toupper(ext[i + 1]);
  }

  while (FAT_ReadEntry(disk, file, &entry)) {
    if (memcmp(fatName, entry.Name, 11) == 0) {
      *entryOut = entry;
      return true;
    }
  }

  return false;
}

FAT_File *FAT_Open(DiskParams *disk, const char *path) {
  char name[MAX_PATH_SIZE];

  if (path[0] == '/')
    path++;

  FAT_File *current = &g_Data->RootDirectory.Public;

  while (*path) {
    bool isLast = false;
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
      isLast = true;
    }

    FAT_DirectoryEntry entry;
    if (FAT_FindFile(disk, current, name, &entry)) {
      FAT_Close(current);

      if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0) {
        debugf("FAT: %s not a directory\n", name);
        return NULL;
      }

      current = FAT_OpenEntry(disk, &entry);
    } else {
      FAT_Close(current);
      debugf("FAT: %s not found\n", name);
      return NULL;
    }
  }

  return current;
}

// High-level function to read entire file into memory
int FAT_ReadFile(const char *path, void *buffer) {
  if (!g_Disk) {
    debugf("ERROR: FAT not initialized!\n");
    return -1;
  }

  debugf("FAT: Opening file '%s'...\n", path);
  FAT_File *file = FAT_Open(g_Disk, path);
  if (!file) {
    debugf("ERROR: Could not open file %s\n", path);
    return -2;
  }

  debugf("FAT: Reading %u bytes...\n", file->Size);
  uint32_t read = FAT_Read(g_Disk, file, file->Size, buffer);

  FAT_Close(file);

  if (read != file->Size) {
    debugf("ERROR: Read %u bytes, expected %u\n", read, file->Size);
    return -3;
  }

  debugf("FAT: File loaded successfully (%u bytes)\n", read);
  return read;
}

#include "fs/fat/fat.h"
#include "arch/i686/errno.h"
#include "klib/ctype.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "mm/kmalloc.h"

/* -----------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static inline void fs_lock(fat_fs_t *fs) {
  if (fs->lock)
    fs->lock(fs);
}

static inline void fs_unlock(fat_fs_t *fs) {
  if (fs->unlock)
    fs->unlock(fs);
}

/* Read exactly one sector into buf.  Uses the per-fs single-sector cache. */
static int read_sector(fat_fs_t *fs, uint32_t lba, void *buf) {
  if (lba == fs->sector_buf_lba && fs->sector_buf) {
    memcpy(buf, fs->sector_buf, fs->bytes_per_sector);
    return 0;
  }
  int rc = fs->dev->read_sectors(fs->dev, lba, 1, buf);
  if (rc < 0)
    return rc;

  /* Update cache */
  if (fs->sector_buf) {
    memcpy(fs->sector_buf, buf, fs->bytes_per_sector);
    fs->sector_buf_lba = lba;
  }
  return 0;
}

/* -----------------------------------------------------------------------
 * Cluster chain helpers
 * ---------------------------------------------------------------------- */

/* Read the next cluster from the in-memory FAT table. */
static uint32_t fat_next_cluster(fat_fs_t *fs, uint32_t cluster) {
  uint32_t val;
  switch (fs->type) {
  case FAT_TYPE_12: {
    uint32_t idx = cluster + cluster / 2; /* idx = cluster * 1.5 */
    if (idx + 1 >= fs->fat_table_bytes)
      return 0xFFFFFFFFu;
    uint16_t raw =
        (uint16_t)fs->fat_table[idx] | ((uint16_t)fs->fat_table[idx + 1] << 8);
    val = (cluster & 1) ? (raw >> 4) : (raw & 0x0FFF);
    return (val >= FAT12_EOC) ? 0xFFFFFFFFu : val;
  }
  case FAT_TYPE_16: {
    uint32_t idx = cluster * 2;
    if (idx + 1 >= fs->fat_table_bytes)
      return 0xFFFFFFFFu;
    val =
        (uint16_t)fs->fat_table[idx] | ((uint16_t)fs->fat_table[idx + 1] << 8);
    return (val >= FAT16_EOC) ? 0xFFFFFFFFu : val;
  }
  case FAT_TYPE_32: {
    uint32_t idx = cluster * 4;
    if (idx + 3 >= fs->fat_table_bytes)
      return 0xFFFFFFFFu;
    val = (uint32_t)fs->fat_table[idx] |
          ((uint32_t)fs->fat_table[idx + 1] << 8) |
          ((uint32_t)fs->fat_table[idx + 2] << 16) |
          ((uint32_t)fs->fat_table[idx + 3] << 24);
    val &= FAT32_MASK;
    return (val >= (FAT32_EOC & FAT32_MASK)) ? 0xFFFFFFFFu : val;
  }
  }
  return 0xFFFFFFFFu;
}

/* Convert cluster number to its first LBA. */
static inline uint32_t cluster_to_lba(fat_fs_t *fs, uint32_t cluster) {
  return fs->data_lba + (cluster - 2) * fs->sectors_per_cluster;
}

/* -----------------------------------------------------------------------
 * FAT detection and mount
 * ---------------------------------------------------------------------- */

fat_fs_t *fat_mount(blkdev_t *dev) {
  /* Read boot sector into a temporary stack buffer */
  uint8_t boot[FAT_SECTOR_SIZE];
  if (dev->read_sectors(dev, 0, 1, boot) < 0) {
    kdebugf("fat_mount: boot sector read failed\n");
    return NULL;
  }

  FAT_BPB *bpb = (FAT_BPB *)boot;

  if (bpb->bytes_per_sector == 0 || bpb->sectors_per_cluster == 0 ||
      bpb->reserved_sectors == 0 || bpb->fat_count == 0) {
    kdebugf("fat_mount: invalid BPB\n");
    return NULL;
  }

  fat_fs_t *fs = kmalloc(sizeof(fat_fs_t));
  if (!fs)
    return NULL;
  memset(fs, 0, sizeof(fat_fs_t));
  fs->dev = dev;
  fs->sector_buf_lba = 0xFFFFFFFFu;

  /* Allocate sector cache */
  fs->sector_buf = kmalloc(FAT_SECTOR_SIZE);
  if (!fs->sector_buf)
    goto err;

  fs->bytes_per_sector = bpb->bytes_per_sector;
  fs->sectors_per_cluster = bpb->sectors_per_cluster;
  fs->bytes_per_cluster = fs->bytes_per_sector * fs->sectors_per_cluster;
  fs->fat_count = bpb->fat_count;
  fs->fat_lba = bpb->reserved_sectors;

  uint32_t total_sectors =
      bpb->total_sectors_16 ? bpb->total_sectors_16 : bpb->total_sectors_32;
  fs->total_sectors = total_sectors;

  uint32_t fat_sectors_16 = bpb->sectors_per_fat_16;
  uint32_t fat_sectors_32 = ((FAT32_ExtBPB *)boot)->sectors_per_fat_32;
  fs->fat_sectors = fat_sectors_16 ? fat_sectors_16 : fat_sectors_32;

  /* Root directory */
  uint32_t root_dir_sectors =
      ((uint32_t)bpb->root_entry_count * 32 + fs->bytes_per_sector - 1) /
      fs->bytes_per_sector;
  fs->root_lba = fs->fat_lba + fs->fat_sectors * fs->fat_count;
  fs->root_sectors = root_dir_sectors;
  fs->data_lba = fs->root_lba + root_dir_sectors;

  fs->cluster_count = (total_sectors - fs->data_lba) / fs->sectors_per_cluster;

  /* Determine FAT type by cluster count (Microsoft's official algorithm) */
  if (fs->cluster_count < 4085)
    fs->type = FAT_TYPE_12;
  else if (fs->cluster_count < 65525)
    fs->type = FAT_TYPE_16;
  else
    fs->type = FAT_TYPE_32;

  if (fs->type == FAT_TYPE_32) {
    fs->root_cluster = ((FAT32_ExtBPB *)boot)->root_cluster;
    fs->root_sectors = 0;
  }

  /* Load entire FAT into memory */
  fs->fat_table_bytes = fs->fat_sectors * fs->bytes_per_sector;
  fs->fat_table = kmalloc(fs->fat_table_bytes);
  if (!fs->fat_table)
    goto err;

  if (dev->read_sectors(dev, fs->fat_lba, fs->fat_sectors, fs->fat_table) < 0) {
    kdebugf("fat_mount: FAT read failed\n");
    goto err;
  }

  kdebugf("fat_mount: FAT%d, %u clusters, data_lba=%u\n", (int)fs->type,
          fs->cluster_count, fs->data_lba);
  return fs;

err:
  kfree(fs->fat_table);
  kfree(fs->sector_buf);
  kfree(fs);
  return NULL;
}

void fat_unmount(fat_fs_t *fs) {
  if (!fs)
    return;
  /* Could flush write-back cache here for RW support */
  kfree(fs->fat_table);
  kfree(fs->sector_buf);
  kfree(fs);
}

/* -----------------------------------------------------------------------
 * File I/O primitives
 * ---------------------------------------------------------------------- */

/*
 * Reload the file's internal sector buffer for the current position,
 * advancing the cluster chain as needed.
 */
static int file_ensure_sector(fat_file_t *file) {
  fat_fs_t *fs = file->fs;
  uint32_t lba;

  if (file->is_root_dir && fs->type != FAT_TYPE_32) {
    /* FAT12/16 flat root directory */
    uint32_t sector_idx = file->position / fs->bytes_per_sector;
    lba = fs->root_lba + sector_idx;
  } else {
    lba = cluster_to_lba(fs, file->current_cluster) +
          file->current_sector_in_cluster;
  }

  return fs->dev->read_sectors(fs->dev, lba, 1, file->buf);
}

/*
 * Advance internal cluster/sector tracking after consuming `take` bytes.
 * Returns 0 if more data is available, 1 at end-of-chain, negative on I/O err.
 */
static int file_advance(fat_file_t *file, uint32_t take) {
  fat_fs_t *fs = file->fs;
  uint32_t old_sector = file->position / fs->bytes_per_sector;
  file->position += take;
  uint32_t new_sector = file->position / fs->bytes_per_sector;

  if (new_sector == old_sector)
    return 0; /* Still in the same sector */

  if (file->is_root_dir && fs->type != FAT_TYPE_32) {
    /* FAT12/16 root: just a linear run of sectors, no chain */
    if (file->position >= fs->root_sectors * fs->bytes_per_sector)
      return 1;
    return file_ensure_sector(file);
  }

  /* Advance sector within cluster */
  file->current_sector_in_cluster++;
  if (file->current_sector_in_cluster < fs->sectors_per_cluster) {
    return file_ensure_sector(file);
  }

  /* Crossed a cluster boundary – follow chain */
  file->current_sector_in_cluster = 0;
  file->current_cluster = fat_next_cluster(fs, file->current_cluster);
  if (file->current_cluster == 0xFFFFFFFFu)
    return 1; /* End of chain */

  return file_ensure_sector(file);
}

/* -----------------------------------------------------------------------
 * Directory name matching
 * ---------------------------------------------------------------------- */

/*
 * Convert a normal path component (e.g. "Kernel.elf") into the FAT 8.3
 * packed uppercase format (e.g. "KERNEL  ELF").
 * Returns false if the name cannot be represented in 8.3.
 */
static bool name_to_83(const char *name, char out83[FAT_NAME_LEN]) {
  memset(out83, ' ', FAT_NAME_LEN);
  const char *dot = strchr(name, '.');

  int base_len = dot ? (int)(dot - name) : (int)strlen(name);
  if (base_len > 8)
    return false;

  for (int i = 0; i < base_len; i++) {
    char c = name[i];
    if (c < 0x20 || c == '"' || c == '/' || c == '\\' || c == '[' || c == ']' ||
        c == ':' || c == '|' || c == '<' || c == '>' || c == '+' || c == '=' ||
        c == ';' || c == ',')
      return false;
    out83[i] = (char)toupper((unsigned char)c);
  }

  if (dot) {
    const char *ext = dot + 1;
    int ext_len = (int)strlen(ext);
    if (ext_len > 3)
      return false;
    for (int i = 0; i < ext_len; i++)
      out83[8 + i] = (char)toupper((unsigned char)ext[i]);
  }
  return true;
}

/* -----------------------------------------------------------------------
 * LFN reconstruction
 * ---------------------------------------------------------------------- */

#define LFN_MAX_ENTRIES 20
#define LFN_CHARS_PER 13

/* Decode a UCS-2 LFN entry's characters into an ASCII buffer at `pos`.
 * Returns the number of characters written. */
static int lfn_decode_entry(const FAT_LFNEntry *lfn, char *buf, int buf_size) {
  const uint16_t *parts[3] = {lfn->name1, lfn->name2, lfn->name3};
  const int counts[3] = {5, 6, 2};
  int written = 0;
  for (int p = 0; p < 3 && written < buf_size; p++) {
    for (int i = 0; i < counts[p] && written < buf_size; i++) {
      uint16_t ch = parts[p][i];
      if (ch == 0xFFFF || ch == 0x0000)
        return written;
      /* Lossy: drop non-ASCII (good enough for most filenames) */
      buf[written++] = (ch < 0x80) ? (char)ch : '?';
    }
  }
  return written;
}

/* -----------------------------------------------------------------------
 * Directory scanning
 * ---------------------------------------------------------------------- */

/*
 * Open a raw directory (not path-resolved) from a FAT_DirEntry.
 * For the root directory, pass cluster=0 and is_root_dir=true.
 */
static fat_file_t *open_dir_from_cluster(fat_fs_t *fs, uint32_t first_cluster,
                                         bool is_root_dir) {
  fat_file_t *dir = kmalloc(sizeof(fat_file_t));
  if (!dir)
    return NULL;
  memset(dir, 0, sizeof(fat_file_t));
  dir->fs = fs;
  dir->is_directory = true;
  dir->is_root_dir = is_root_dir;
  dir->first_cluster = first_cluster;
  dir->current_cluster = first_cluster;

  if (file_ensure_sector(dir) < 0) {
    kfree(dir);
    return NULL;
  }
  return dir;
}

bool fat_read_dir(fat_file_t *dir, FAT_DirEntry *out) {
  if (!dir->is_directory)
    return false;
  fat_fs_t *fs = dir->fs;

  while (1) {
    uint32_t offset = dir->position % fs->bytes_per_sector;
    if (offset == 0 && dir->position > 0) {
      /* Need next sector */
      int rc = file_advance(dir, 0);
      /* file_advance with 0 bytes doesn't advance; load current sector */
      (void)rc;
    }

    /* Bounds check for flat root */
    if (dir->is_root_dir && fs->type != FAT_TYPE_32) {
      if (dir->position >= fs->root_sectors * fs->bytes_per_sector)
        return false;
    }

    FAT_DirEntry *entry = (FAT_DirEntry *)(dir->buf + offset);

    if (entry->name[0] == 0x00)
      return false; /* End of directory */

    /* Advance past this entry */
    uint32_t take = sizeof(FAT_DirEntry);
    int rc = file_advance(dir, take);
    (void)rc;

    if ((uint8_t)entry->name[0] == 0xE5)
      continue; /* Deleted */
    if (entry->attributes == FAT_ATTR_LFN)
      continue; /* LFN – handled by lookup */
    if (entry->attributes & FAT_ATTR_VOLUME_ID)
      continue;

    *out = *entry;
    return true;
  }
}

/*
 * Find a directory entry matching `name` (either 8.3 or LFN) within the
 * open directory `dir`.  Resets position to 0 before scanning.
 * Returns true and fills `out` on match.
 */
static bool dir_find(fat_file_t *dir, const char *name, FAT_DirEntry *out) {
  fat_fs_t *fs = dir->fs;

  /* Reset to start of directory */
  dir->position = 0;
  dir->current_cluster = dir->first_cluster;
  dir->current_sector_in_cluster = 0;
  file_ensure_sector(dir);

  char name83[FAT_NAME_LEN];
  bool has83 = name_to_83(name, name83);

  /* LFN accumulation buffer */
  char lfn_buf[LFN_MAX_ENTRIES * LFN_CHARS_PER + 1];
  bool lfn_valid = false;
  memset(lfn_buf, 0, sizeof(lfn_buf));

  while (1) {
    uint32_t offset = dir->position % fs->bytes_per_sector;
    FAT_DirEntry *raw = (FAT_DirEntry *)(dir->buf + offset);

    if (raw->name[0] == 0x00)
      return false;

    uint32_t take = sizeof(FAT_DirEntry);

    if ((uint8_t)raw->name[0] == 0xE5) {
      lfn_valid = false;
      file_advance(dir, take);
      continue;
    }

    if (raw->attributes == FAT_ATTR_LFN) {
      FAT_LFNEntry *lfn = (FAT_LFNEntry *)raw;
      uint8_t seq = lfn->order & 0x1F;
      if (seq < 1 || seq > LFN_MAX_ENTRIES) {
        lfn_valid = false;
        file_advance(dir, take);
        continue;
      }
      int char_offset = (seq - 1) * LFN_CHARS_PER;
      int written = lfn_decode_entry(lfn, lfn_buf + char_offset,
                                     (int)sizeof(lfn_buf) - char_offset);
      (void)written;
      if (lfn->order & 0x40) {
        /* Last LFN entry (first in sequence order) – terminate */
        lfn_buf[char_offset + written] = '\0';
        lfn_valid = true;
      }
      file_advance(dir, take);
      continue;
    }

    if (raw->attributes & FAT_ATTR_VOLUME_ID) {
      lfn_valid = false;
      file_advance(dir, take);
      continue;
    }

    /* We have a real directory entry */
    bool matched = false;
    if (lfn_valid && strcmp(lfn_buf, name) == 0)
      matched = true;
    if (!matched && has83 && memcmp(name83, raw->name, FAT_NAME_LEN) == 0)
      matched = true;

    lfn_valid = false;

    if (matched) {
      *out = *raw;
      return true;
    }

    file_advance(dir, take);
  }
}

/* -----------------------------------------------------------------------
 * Path resolution
 * ---------------------------------------------------------------------- */

/*
 * Walk an absolute path like "/boot/kernel.elf" and return a heap-allocated
 * fat_file_t for the target, or NULL if not found.
 */
fat_file_t *fat_open(fat_fs_t *fs, const char *path) {
  if (!path || path[0] != '/')
    return NULL;
  path++; /* skip leading '/' */

  /* Open root directory */
  fat_file_t *current;
  if (fs->type == FAT_TYPE_32) {
    current = open_dir_from_cluster(fs, fs->root_cluster, false);
  } else {
    current = open_dir_from_cluster(fs, fs->root_lba, true);
  }
  if (!current)
    return NULL;

  /* Empty path → caller wants the root directory */
  if (*path == '\0')
    return current;

  char component[FAT_MAX_PATH];

  while (*path) {
    /* Extract next path component */
    const char *sep = strchr(path, '/');
    size_t len = sep ? (size_t)(sep - path) : strlen(path);
    if (len == 0 || len >= FAT_MAX_PATH) {
      fat_close(current);
      return NULL;
    }
    memcpy(component, path, len);
    component[len] = '\0';
    path += len;
    if (*path == '/')
      path++;
    bool is_last = (*path == '\0');

    /* Require current to be a directory */
    if (!current->is_directory) {
      fat_close(current);
      return NULL;
    }

    FAT_DirEntry entry;
    if (!dir_find(current, component, &entry)) {
      kdebugf("fat_open: '%s' not found\n", component);
      fat_close(current);
      return NULL;
    }

    bool is_dir = (entry.attributes & FAT_ATTR_DIRECTORY) != 0;

    if (!is_last && !is_dir) {
      kdebugf("fat_open: '%s' is not a directory\n", component);
      fat_close(current);
      return NULL;
    }

    /* Build fat_file_t for this entry */
    fat_file_t *next = kmalloc(sizeof(fat_file_t));
    if (!next) {
      fat_close(current);
      return NULL;
    }
    memset(next, 0, sizeof(fat_file_t));
    next->fs = fs;
    next->is_directory = is_dir;
    next->size = entry.size;
    next->first_cluster = (uint32_t)entry.first_cluster_low |
                          ((uint32_t)entry.first_cluster_high << 16);
    next->current_cluster = next->first_cluster;
    next->current_sector_in_cluster = 0;

    if (file_ensure_sector(next) < 0) {
      kfree(next);
      fat_close(current);
      return NULL;
    }

    fat_close(current);
    current = next;
  }

  return current;
}

ssize_t fat_read(fat_file_t *file, void *buf, size_t size) {
  if (file->is_directory)
    return -EISDIR;
  fat_fs_t *fs = file->fs;

  /* Clamp to remaining file bytes */
  if (file->position + (uint32_t)size > file->size)
    size = file->size - file->position;

  uint8_t *out = buf;
  size_t remaining = size;

  while (remaining > 0) {
    uint32_t sector_off = file->position % fs->bytes_per_sector;
    uint32_t avail = fs->bytes_per_sector - sector_off;
    uint32_t take = (uint32_t)(remaining < avail ? remaining : avail);

    memcpy(out, file->buf + sector_off, take);
    out += take;
    remaining -= take;

    if (take == avail && remaining > 0) {
      int rc = file_advance(file, take);
      if (rc < 0)
        return -EIO;
      if (rc > 0)
        break; /* EOF (end of chain) */
    } else {
      file->position += take;
    }
  }

  return (ssize_t)(out - (uint8_t *)buf);
}

off_t fat_seek(fat_file_t *file, off_t offset, int whence) {
  fat_fs_t *fs = file->fs;
  int64_t target;

  switch (whence) {
  case 0:
    target = offset;
    break;
  case 1:
    target = (int64_t)file->position + offset;
    break;
  case 2:
    target = (int64_t)file->size + offset;
    break;
  default:
    return (off_t)-EINVAL;
  }

  if (target < 0)
    return (off_t)-EINVAL;
  if (!file->is_directory && (uint32_t)target > file->size)
    target = file->size;

  uint32_t new_pos = (uint32_t)target;

  /* If seeking backwards, restart from first cluster */
  if (new_pos < file->position) {
    file->position = 0;
    file->current_cluster = file->first_cluster;
    file->current_sector_in_cluster = 0;
    file_ensure_sector(file);
  }

  /* Advance forward sector-by-sector */
  while (file->position < new_pos) {
    uint32_t sector_off = file->position % fs->bytes_per_sector;
    uint32_t avail = fs->bytes_per_sector - sector_off;
    uint32_t step = new_pos - file->position;
    if (step > avail)
      step = avail;

    if (step == avail) {
      int rc = file_advance(file, step);
      if (rc < 0)
        return (off_t)-EIO;
      if (rc > 0)
        break;
    } else {
      file->position += step;
    }
  }

  return (off_t)file->position;
}

void fat_close(fat_file_t *file) {
  if (file)
    kfree(file);
}

int fat_stat(fat_fs_t *fs, const char *path, FAT_DirEntry *out) {
  fat_file_t *f = fat_open(fs, path);
  if (!f)
    return -ENOENT;

  /* Reconstruct a synthetic DirEntry from the fat_file_t */
  memset(out, 0, sizeof(FAT_DirEntry));
  out->size = f->size;
  out->first_cluster_low = (uint16_t)(f->first_cluster & 0xFFFF);
  out->first_cluster_high = (uint16_t)(f->first_cluster >> 16);
  out->attributes = f->is_directory ? FAT_ATTR_DIRECTORY : FAT_ATTR_ARCHIVE;
  fat_close(f);
  return 0;
}

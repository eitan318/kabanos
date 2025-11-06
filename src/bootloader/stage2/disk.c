#include "disk.h"
#include "stdio.h"

struct DiskAddressPacket {
  uint8_t size;                     // must be 0x10
  uint8_t reserved;                 // always 0
  uint16_t sector_count;            // up to 128 (0x80)
  uint16_t transfer_buffer_offset;  // offset in segment
  uint16_t transfer_buffer_segment; // segment
  uint32_t start_lba_low;           // lower 32 bits of LBA
  uint32_t start_lba_high;          // upper 32 bits of LBA
} __attribute__((packed, aligned(1)));

#define DAP_SIZE 0x10

extern bool __attribute__((cdecl))
bios_read_lba(uint8_t drive_number, struct DiskAddressPacket *dap);

extern bool __attribute__((cdecl))
bios_read_chs(uint8_t drive_number, uint16_t cylinder, uint16_t head,
              uint16_t sector, uint16_t count, void *dest);

extern bool __attribute__((cdecl)) bios_check_lba_support(uint8_t disk_number);
extern bool __attribute__((cdecl))
bios_get_drive_params(uint8_t drive, uint8_t *drive_type, uint8_t *HDDs_count,
                      uint16_t *cylinders, uint16_t *heads, uint16_t *sectors);

extern bool __attribute__((cdecl)) bios_disk_reset(uint8_t disk_number);

bool disk_init(uint8_t drive_number, DiskParams *disk_params) {
  if (!disk_params)
    return false;

  // Check LBA support
  disk_params->lba_support = bios_check_lba_support(drive_number);
  disk_params->drive_id = drive_number;

  // Get CHS info
  uint8_t hdds_count = 0;
  uint8_t disk_type = 0;
  uint16_t heads_count = 0;
  uint16_t sectors_per_track = 0;
  uint16_t cylinders_count = 0;
  if (!bios_get_drive_params(drive_number, &disk_type, &hdds_count,
                             &cylinders_count, &heads_count,
                             &sectors_per_track)) {
    return false;
  }

  disk_params->heads = heads_count;
  disk_params->sectors = sectors_per_track;
  disk_params->cylinders = cylinders_count;
  disk_params->hdds_count = hdds_count;

  // printf("CHS=%u:%u:%u\n", disk_params->cylinders, disk_params->heads,
  //      disk_params->sectors);

  // Print drive info
  return true;
}

static void disk_lba_to_chs(const DiskParams *disk_params, uint32_t lba,
                            uint16_t *cylinder_out, uint16_t *head_out,
                            uint16_t *sector_out) {
  // sector = (LBA % sectors per track + 1)
  *sector_out = lba % disk_params->sectors + 1;

  // cylinder = (LBA / sectors per track) / heads
  *cylinder_out = (lba / disk_params->sectors) / disk_params->heads;

  // head = (LBA / sectors per track) % heads
  *head_out = (lba / disk_params->sectors) % disk_params->heads;
}

bool disk_read_sectors(const DiskParams *disk_params, uint32_t lba,
                       uint16_t count, void *dest) {
  const int reread = 3;
  uint8_t *buffer = (uint8_t *)dest;
  if (disk_params->lba_support) {
    struct DiskAddressPacket dap = {
        .size = DAP_SIZE,
        .reserved = 0,
        .sector_count = count,
        .transfer_buffer_offset = ((uintptr_t)buffer) & 0xF,
        .transfer_buffer_segment = ((uintptr_t)buffer >> 4) & 0xFFFF,
        .start_lba_low = lba,
        .start_lba_high = 0};

    for (int i = 0; i < reread; i++) {
      debugf("Loading %u sectors from LBA=%u\n", count,
             lba); // Fixed format string
      if (bios_read_lba(disk_params->drive_id, &dap)) {
        return true;
      }
      bios_disk_reset(disk_params->drive_id);
    }
  } else {
    uint16_t cylinder = 0, head = 0, sector = 0;
    disk_lba_to_chs(disk_params, lba, &cylinder, &head, &sector);
    for (int i = 0; i < reread; i++) {
      debugf("Loading %u sectors from CHS=%u:%u:%u\n", count, cylinder, head,
             sector);
      if (bios_read_chs(disk_params->drive_id, cylinder, head, sector, count,
                        buffer)) {
        return true;
      }
      bios_disk_reset(disk_params->drive_id);
    }
  }
  return false;
}

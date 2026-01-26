#include "ata.h"
#include "arch/i686/ata_portmap.h"
#include "hal.h"

// ATA Commands
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_CACHE_FLUSH 0xE7

// ATA Status Bits
#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_SRV 0x10
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_RDY 0x40
#define ATA_STATUS_BSY 0x80

static void ata_delay_400ns(void) {
  for (int i = 0; i < 4; i++) {
    hal_in8(ATA_PRIMARY_STATUS);
  }
}

static int ata_wait_not_busy(void) {
  uint8_t status;
  int timeout = 1000000;

  while (timeout--) {
    status = hal_in8(ATA_PRIMARY_STATUS);
    if (!(status & ATA_STATUS_BSY)) {
      return 0;
    }
  }
  return -1; // Timeout
}

// Wait until the drive is ready
static int ata_wait_ready(void) {
  uint8_t status;

  int timeout = 1000000;

  if (ata_wait_not_busy() != 0) {
    return -1;
  }

  while (timeout--) {
    status = hal_in8(ATA_PRIMARY_STATUS);
    if (status & ATA_STATUS_RDY) {
      return 0;
    }
  }
  return -1; // Timeout
}

// Wait for data request
static int ata_wait_drq(void) {
  uint8_t status;
  int timeout = 1000000;

  if (ata_wait_not_busy() != 0) {
    return -1;
  }

  while (timeout--) {
    status = hal_in8(ATA_PRIMARY_STATUS);
    if (status & ATA_STATUS_DRQ) {
      return 0;
    }
    if (status & ATA_STATUS_ERR) {
      return -2; // Error
    }
  }
  return -1; // Timeout
}

// Read a sector from disk
void ata_read_sector(uint32_t lba, int count, uint8_t *buffer) {
  if (count == 0 || count > 256) {
    return;
  }

  // Wait for drive to be ready
  if (ata_wait_ready() != 0) {
    return;
  }

  // Select drive (master) and set LBA mode with highest 4 bits of LBA
  hal_out8(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
  ata_delay_400ns();

  // Send sector count (0 means 256 sectors)
  hal_out8(ATA_PRIMARY_SECCOUNT, count);

  // Send LBA address
  hal_out8(ATA_PRIMARY_LBA_LO, lba & 0xFF);
  hal_out8(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
  hal_out8(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);

  // Send read command
  hal_out8(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

  // Read all sectors
  for (int i = 0; i < count; i++) {
    // Wait for data to be ready
    if (ata_wait_drq() != 0) {
      return;
    }

    // Read 256 words (512 bytes)
    uint16_t *buf16 = (uint16_t *)(buffer + i * SECTOR_SIZE);
    for (int j = 0; j < 256; j++) {
      buf16[j] = hal_in16(
          ATA_PRIMARY_DATA); // FIXED: was io_read8, should be io_read16
    }

    ata_delay_400ns();
  }
}

// Write a sector to disk
int ata_write_sector(uint32_t lba, int count, const uint8_t *buffer) {
  if (count == 0 || count > 256) {
    return -1;
  }

  // Wait for drive to be ready
  if (ata_wait_ready() != 0) {
    return -1;
  }

  // Select drive (master) and set LBA mode with highest 4 bits of LBA
  hal_out8(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
  ata_delay_400ns();

  // Send sector count (0 means 256 sectors)
  hal_out8(ATA_PRIMARY_SECCOUNT, count);

  // Send LBA address
  hal_out8(ATA_PRIMARY_LBA_LO, lba & 0xFF);
  hal_out8(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
  hal_out8(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);

  // Send write command
  hal_out8(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

  // Write all sectors
  for (int i = 0; i < count; i++) {
    // Wait for drive to request data
    if (ata_wait_drq() != 0) {
      return -1;
    }

    // Write 256 words (512 bytes)
    const uint16_t *buf16 = (const uint16_t *)(buffer + i * SECTOR_SIZE);
    for (int j = 0; j < 256; j++) {
      hal_out16(ATA_PRIMARY_DATA, buf16[j]);
    }

    ata_delay_400ns();
  }

  // Flush cache to ensure data is written to disk
  hal_out8(ATA_PRIMARY_COMMAND, ATA_CMD_CACHE_FLUSH);
  ata_wait_not_busy();
  return 0;
}

// Initialize ATA driver
void ata_init(void) {
  // Select master drive
  hal_out8(ATA_PRIMARY_DRIVE_HEAD, 0xE0);
  ata_delay_400ns();

  // Wait for drive to be ready
  ata_wait_ready();
}

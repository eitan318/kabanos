/**
 * @file ata_portmap.h
 * @brief I/O port numbers of the primary ATA channel.
 */
#pragma once
#include "klib/stdint.h"

/* Primary ATA I/O ports */
#define ATA_PRIMARY_DATA 0x1F0       /**< Data register (16-bit). */
#define ATA_PRIMARY_ERROR 0x1F1      /**< Error register (read). */
#define ATA_PRIMARY_FEATURES 0x1F1   /**< Features register (write). */
#define ATA_PRIMARY_SECCOUNT 0x1F2   /**< Sector count. */
#define ATA_PRIMARY_LBA_LO 0x1F3     /**< LBA low byte. */
#define ATA_PRIMARY_LBA_MID 0x1F4    /**< LBA mid byte. */
#define ATA_PRIMARY_LBA_HI 0x1F5     /**< LBA high byte. */
#define ATA_PRIMARY_DRIVE_HEAD 0x1F6 /**< Drive/head select. */
#define ATA_PRIMARY_STATUS 0x1F7     /**< Status register (read). */
#define ATA_PRIMARY_COMMAND 0x1F7    /**< Command register (write). */

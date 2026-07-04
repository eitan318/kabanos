/**
 * @file pci.h
 * @brief PCI configuration-space access (port I/O mechanism #1).
 */
#pragma once
#include <klib/stdint.h>

/**
 * @brief Reads a 32-bit value from a device's configuration space.
 * @param offset Register offset; must be 4-byte aligned.
 */
uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t slot,
    uint8_t func,
    uint8_t offset
);

/**
 * @brief Writes a 32-bit value into a device's configuration space.
 * @param offset Register offset; must be 4-byte aligned.
 */
void pci_config_write32(
    uint8_t bus,
    uint8_t slot,
    uint8_t func,
    uint8_t offset,
    uint32_t value
);

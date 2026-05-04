#include "drivers/pci/pci.h"
#include "hal.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t slot,
    uint8_t func,
    uint8_t offset
) {
    uint32_t address =
        (1U << 31) |
        ((uint32_t)bus  << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)func << 8)  |
        (offset & 0xFC);

    hal_out32(PCI_CONFIG_ADDRESS, address);
    return hal_in32(PCI_CONFIG_DATA);
}

void pci_config_write32(
    uint8_t bus,
    uint8_t slot,
    uint8_t func,
    uint8_t offset,
    uint32_t value
) {
    uint32_t address =
        (1U << 31) |
        ((uint32_t)bus  << 16) |
        ((uint32_t)slot << 11) |
        ((uint32_t)func << 8)  |
        (offset & 0xFC);

    hal_out32(PCI_CONFIG_ADDRESS, address);
    hal_out32(PCI_CONFIG_DATA, value);
}

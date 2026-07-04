/**
 * @file rtl8139.h
 * @brief Realtek RTL8139 network card driver.
 */
#pragma once
#include <klib/stdbool.h>
#include <klib/stdint.h>
#include "mm/vmspace.h"
#include "modules.h"

/** @brief Module entry point: finds the NIC on the PCI bus and brings it up. */
void rtl8139_init(module_t *module);

void rtl8139_disable();

/** @brief Soft-resets the chip and reprograms the receive buffer. */
void rtl8139_reset();

/** @brief Transmits one raw Ethernet frame. */
void rtl8139_send_packet(uint8_t *data, uint32_t length);

/**
 * @brief Copies the next received frame out of the RX ring.
 * @return Frame length in bytes, or 0 if nothing was received.
 */
uint32_t rtl8139_receive_packet(uint8_t *buffer, uint32_t buffer_len);

/**
 * @brief Peeks the oldest buffered frame without consuming it.
 * @return Frame length in bytes, or 0 if the buffer is empty.
 */
uint32_t rtl8139_get_packet(uint8_t *buffer, uint32_t max_len);

/** @brief Consumes the frame returned by rtl8139_get_packet(). */
void rtl8139_clear_packet();

/** @brief True if a received frame is waiting. */
bool rtl8139_has_packet();

/** @brief Sets the PCI bus-mastering bit so the NIC can DMA. */
void rtl8139_enable_bus_mastering(uint8_t bus, uint8_t slot);

/** @brief Enables/disables hardware loopback mode (for testing). */
void rtl8139_set_loopback(bool enable);

/** @brief Hex-dumps a frame to the console (debug helper). */
void rtl8139_packet_print(uint8_t *data, uint32_t len);

/** @brief Prints a MAC address in the usual colon-separated form. */
void print_mac(uint8_t *mac);

/* Network configuration */
void rtl8139_print_network_config();
uint8_t *rtl8139_get_ip();
uint8_t *rtl8139_get_mac();
uint8_t *rtl8139_get_gateway();

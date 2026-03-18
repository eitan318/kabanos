#pragma once
#include <klib/stdbool.h>
#include <klib/stdint.h>
#include "mm/vmspace.h"
#include "modules.h"

void rtl8139_init(module_t *module);
void rtl8139_disable();
void rtl8139_reset();
void rtl8139_send_packet(uint8_t *data, uint32_t length);
uint32_t rtl8139_receive_packet(uint8_t *buffer, uint32_t buffer_len);

uint32_t rtl8139_get_packet(uint8_t *buffer, uint32_t max_len);
void rtl8139_clear_packet();
bool rtl8139_has_packet();

void rtl8139_enable_bus_mastering(uint8_t bus, uint8_t slot);

void rtl8139_set_loopback(bool enable);
void rtl8139_packet_print(uint8_t *data, uint32_t len);
void print_mac(uint8_t *mac);

// Network configuration functions
void rtl8139_print_network_config();
uint8_t* rtl8139_get_ip();
uint8_t* rtl8139_get_mac();
uint8_t* rtl8139_get_gateway(); 

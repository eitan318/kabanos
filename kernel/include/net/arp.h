#pragma once
#include <klib/stdint.h>
#include <klib/stdbool.h>
#include "protocols.h"

// ARP packet structure (28 bytes)
typedef struct __attribute__((packed)) {
    uint16_t hardware_type;      // 1 for Ethernet
    uint16_t protocol_type;      // 0x0800 for IPv4
    uint8_t  hardware_addr_len;  // 6 for MAC
    uint8_t  protocol_addr_len;  // 4 for IPv4
    uint16_t operation;          // 1 = request, 2 = reply
    uint8_t  src_mac[6];
    uint8_t  src_ip[4];
    uint8_t  dst_mac[6];
    uint8_t  dst_ip[4];
} arp_packet_t;

// ARP operations
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

// Ethernet + ARP = 14 bytes + 28 bytes = 42 bytes
#define ARP_PACKET_SIZE 42

bool check_arp_is_for_us(arp_packet_t *arp, uint8_t *our_ip);
void arp_send_reply(arp_packet_t *request, uint8_t *our_mac, uint8_t *our_ip);
void arp_send_request(uint8_t *target_ip, uint8_t *our_mac, uint8_t *our_ip);
bool arp_is_reply(uint8_t *packet_buffer, uint32_t len, uint8_t *out_mac);
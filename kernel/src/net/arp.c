/**
 * @file arp.c
 * @brief ARP request/reply handling and the ARP cache.
 */
#include "net/arp.h"
#include "drivers/rtl8139/rtl8139.h"
#include "klib/string.h"
#include "klib/stdio.h"
#include "sched/sched.h"

static arp_cache_entry_t g_arp_cache[ARP_CACHE_SIZE];
static int g_arp_cache_next = 0; // round-robin eviction

bool arp_cache_lookup(uint8_t *ip, uint8_t *out_mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (g_arp_cache[i].valid &&
            memcmp(g_arp_cache[i].ip, ip, 4) == 0) {
            memcpy(out_mac, g_arp_cache[i].mac, 6);
            return true;
        }
    }
    return false;
}

void arp_cache_insert(uint8_t *ip, uint8_t *mac) {
    // update existing entry if present
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (g_arp_cache[i].valid &&
            memcmp(g_arp_cache[i].ip, ip, 4) == 0) {
            memcpy(g_arp_cache[i].mac, mac, 6);
            return;
        }
    }
    // insert into next slot (round-robin)
    arp_cache_entry_t *e = &g_arp_cache[g_arp_cache_next];
    memcpy(e->ip,  ip,  4);
    memcpy(e->mac, mac, 6);
    e->valid = true;
    g_arp_cache_next = (g_arp_cache_next + 1) % ARP_CACHE_SIZE;
}

bool check_arp_is_for_us(arp_packet_t *arp, uint8_t *our_ip) {
    if (ntohs(arp->operation) != ARP_OP_REQUEST) {
        return false;
    }

    return (arp->dst_ip[0] == our_ip[0] &&
           arp->dst_ip[1] == our_ip[1] &&
           arp->dst_ip[2] == our_ip[2] &&
           arp->dst_ip[3] == our_ip[3]);
}

void arp_send_reply(arp_packet_t *request, uint8_t *our_mac, uint8_t *our_ip) {
    uint8_t reply_packet[ARP_PACKET_SIZE];

    // ===== Ethernet Header =====
    ether_header_t *eth = (ether_header_t*)reply_packet;
    memcpy(eth->ether_dst_mac, request->src_mac, 6);
    memcpy(eth->ether_src_mac, our_mac, 6);
    eth->ether_type[0] = 0x08;
    eth->ether_type[1] = 0x06;

    // ===== ARP Body =====
    arp_packet_t *arp = (arp_packet_t*)(&reply_packet[14]);
    arp->hardware_type = htons(1);
    arp->protocol_type = htons(ETHER_TYPE_IPV4);
    arp->hardware_addr_len = 6;
    arp->protocol_addr_len = 4;
    arp->operation = htons(ARP_OP_REPLY);
    memcpy(arp->src_mac, our_mac, 6);
    memcpy(arp->src_ip, our_ip, 4);
    memcpy(arp->dst_mac, request->src_mac, 6);
    memcpy(arp->dst_ip, request->src_ip,  4);

    rtl8139_send_packet(reply_packet, ARP_PACKET_SIZE);
}

void arp_send_request(uint8_t *target_ip, uint8_t *our_mac, uint8_t *our_ip) {
    uint8_t request_packet[ARP_PACKET_SIZE];
    memset(request_packet, 0, ARP_PACKET_SIZE);

    // ===== Ethernet Header =====
    ether_header_t *eth = (ether_header_t*)request_packet;
    memset(eth->ether_dst_mac, 0xFF, 6);
    memcpy(eth->ether_src_mac, our_mac, 6);
    eth->ether_type[0] = 0x08;
    eth->ether_type[1] = 0x06;

    // ===== ARP Body =====
    arp_packet_t *arp = (arp_packet_t*)(&request_packet[ETHER_HEADER_SIZE]);
    arp->hardware_type = htons(1);
    arp->protocol_type = htons(ETHER_TYPE_IPV4);
    arp->hardware_addr_len = 6;
    arp->protocol_addr_len = 4;
    arp->operation = htons(ARP_OP_REQUEST);
    memcpy(arp->src_mac, our_mac, 6);
    memcpy(arp->src_ip,  our_ip, 4);
    memset(arp->dst_mac, 0xFF, 6);
    memcpy(arp->dst_ip,  target_ip, 4);

    rtl8139_send_packet(request_packet, ARP_PACKET_SIZE);
}

bool arp_is_reply(uint8_t *packet_buffer, uint32_t len, uint8_t *out_mac) {
    if (len < ETHER_HEADER_SIZE + (int)sizeof(arp_packet_t)) {
	  return false;
	}

    uint16_t ethertype = (packet_buffer[12] << 8) | packet_buffer[13];
    if (ethertype != ETHER_TYPE_ARP) {
	  return false;
	}

    arp_packet_t *arp = (arp_packet_t*)(&packet_buffer[ETHER_HEADER_SIZE]);
    if (ntohs(arp->operation) != ARP_OP_REPLY) {
	  return false;
	}

    memcpy(out_mac, arp->src_mac, 6);
    return true;
}

bool arp_resolve(uint8_t *target_ip, uint8_t *our_mac,
                 uint8_t *our_ip, uint8_t *out_mac) {
    if (arp_cache_lookup(target_ip, out_mac))
        return true;

    for (int tries = 0; tries < 15; tries++) {
        if (tries % 5 == 0)
            arp_send_request(target_ip, our_mac, our_ip);

        sys_yield();  // yield so the ISR can fire and populate the cache

        if (arp_cache_lookup(target_ip, out_mac))
            return true;
    }
    return false;
}
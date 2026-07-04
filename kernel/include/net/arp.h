/**
 * @file arp.h
 * @brief ARP (Address Resolution Protocol) handling and cache.
 */
#pragma once
#include <klib/stdint.h>
#include <klib/stdbool.h>
#include "protocols.h"

/** @brief ARP packet body for Ethernet/IPv4 (28 bytes). */
typedef struct __attribute__((packed)) {
    uint16_t hardware_type;      /**< 1 for Ethernet. */
    uint16_t protocol_type;      /**< 0x0800 for IPv4. */
    uint8_t  hardware_addr_len;  /**< 6 for MAC. */
    uint8_t  protocol_addr_len;  /**< 4 for IPv4. */
    uint16_t operation;          /**< ARP_OP_REQUEST or ARP_OP_REPLY. */
    uint8_t  src_mac[6];
    uint8_t  src_ip[4];
    uint8_t  dst_mac[6];
    uint8_t  dst_ip[4];
} arp_packet_t;

/* ARP cache */
#define ARP_CACHE_SIZE 16

/** @brief One IP-to-MAC binding in the ARP cache. */
typedef struct {
    uint8_t ip[4];
    uint8_t mac[6];
    bool valid;
} arp_cache_entry_t;

/* ARP operations */
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

/** @brief Full frame size: Ethernet header (14) + ARP packet (28). */
#define ARP_PACKET_SIZE 42

/** @brief True if @p arp is a request targeting @p our_ip. */
bool check_arp_is_for_us(arp_packet_t *arp, uint8_t *our_ip);

/** @brief Answers an ARP request with our MAC address. */
void arp_send_reply(arp_packet_t *request, uint8_t *our_mac, uint8_t *our_ip);

/** @brief Broadcasts an ARP request for @p target_ip. */
void arp_send_request(uint8_t *target_ip, uint8_t *our_mac, uint8_t *our_ip);

/**
 * @brief Checks whether a raw frame is an ARP reply; if so, extracts the
 *        sender's MAC into @p out_mac.
 */
bool arp_is_reply(uint8_t *packet_buffer, uint32_t len, uint8_t *out_mac);

/**
 * @brief Resolves @p target_ip to a MAC address, using the cache or by
 *        sending a request and waiting for the reply.
 * @return true on success; @p out_mac receives the MAC address.
 */
bool arp_resolve(uint8_t *target_ip, uint8_t *our_mac,
                 uint8_t *our_ip, uint8_t *out_mac);

/* Cache API */

/** @brief Looks up @p ip in the cache; fills @p out_mac on a hit. */
bool arp_cache_lookup(uint8_t *ip, uint8_t *out_mac);

/** @brief Inserts or refreshes a cache entry. */
void arp_cache_insert(uint8_t *ip, uint8_t *mac);

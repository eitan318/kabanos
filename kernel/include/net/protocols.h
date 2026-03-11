#pragma once
#include <klib/stdint.h>
#include <klib/stdbool.h>

// Ethernet header structure (14 bytes)
typedef struct __attribute__((packed)) {
    uint8_t ether_dst_mac[6];
    uint8_t ether_src_mac[6];
    uint8_t ether_type[2];
} ether_header_t;

#define ETHER_HEADER_SIZE 14
#define MAX_ETHER_PACKET_SIZE 1500

#define ETHER_TYPE_CUSTOM 0x9999
#define ETHER_TYPE_IPV4 0X0800
#define ETHER_TYPE_ARP 0X0806

// IPv4 header structure (20 bytes without options)
typedef struct __attribute__((packed)) {
    uint8_t version_hl;      // 4 bits version, 4 bits header length
	uint8_t service_type;
	uint16_t total_length;   // total packet size
	uint16_t identification;
	uint16_t flags_fragment; // 3 bits of flags, 13 bits fragment offset
	uint8_t ttl;
	uint8_t protocol; 		 // type of protocol (icmp, tcp, udp, etc.)
	uint16_t header_checksum;
	uint8_t src_ip[4];
	uint8_t dst_ip[4];
} ipv4_header_t;

#define IPV4_HEADER_SIZE 20
#define DEFAULT_TTL 128

#define ICMP_PROTOCOL 1

// Helper function to convert network byte order (big endian) to little endian
static inline uint16_t ntohs(uint16_t netshort) {
    return (netshort >> 8) | (netshort << 8);
}

// Helper function to convert little endian order to network byte order (big endian)
static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

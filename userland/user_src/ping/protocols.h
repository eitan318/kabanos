#pragma once
#include <stdint.h>
#include <stdbool.h>

// Ethernet header structure (14 bytes)
typedef struct __attribute__((packed)) {
    uint8_t ether_dst_mac[6];
    uint8_t ether_src_mac[6];
    uint8_t ether_type[2];
} ether_header_t;

#define ETHER_HEADER_SIZE 14
#define MAX_PACKET_SIZE 1514

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

#define ARP_HEADER_SIZE 28

// ARP operations
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

// Ethernet + ARP = 14 bytes + 28 bytes = 42 bytes
#define ARP_PACKET_SIZE 42

// ICMP packet structure (8 bytes)
typedef struct __attribute__((packed)) {
    uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
} icmp_packet_t;

#define ICMP_HEADER_SIZE 8
#define OUR_IDENTIFIER 0x4321

#define ICMP_ECHO_REPLY 0
#define ICMP_DST_UNREACHABLE 3
#define ICMP_REDIRECT_MSG 5
#define ICMP_ECHO_REQUEST 8
#define ICMP_ROUTER_AD 9
#define ICMP_ROUTER_SOLICITATION 10
#define ICMP_TIME_EXCEEDED 11
#define ICMP_PARAM_PROBLEM 12
#define ICMP_TIMESTAMP 13
#define ICMP_TIMESTAMP_REPLY 14

// Helper function to convert network byte order (big endian) to little endian
static inline uint16_t ntohs(uint16_t netshort) {
    return (netshort >> 8) | (netshort << 8);
}

// Helper function to convert little endian order to network byte order (big endian)
static inline uint16_t htons(uint16_t hostshort) {
    return (hostshort >> 8) | (hostshort << 8);
}

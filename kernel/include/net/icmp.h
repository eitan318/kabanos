#pragma once
#include <klib/stdint.h>
#include <klib/stdbool.h>
#include "protocols.h"

// ICMP packet structure (8 bytes)
typedef struct __attribute__((packed)) {
    uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
} icmp_packet_t;

#define ICMP_HEADER_SIZE 8

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

#define OUR_PING_IDENTIFIER 0x4321
#define PING_COUNT 4
#define PING_TIMEOUT_MS 2000
#define PING_INTERVAL_MS 2000
#define PING_DATA_LENGTH 17

// Check if this is a ping request for us
bool icmp_is_ping_request(ipv4_header_t *ip_header, uint8_t *our_ip);

// Send ping reply
void icmp_send_ping_reply(uint8_t *packet_buffer, uint32_t packet_len, 
                          uint8_t *our_mac, uint8_t *our_ip);

// Send ping request
bool icmp_send_echo_request(uint8_t *target_ip, uint16_t sequence, uint8_t *our_mac, 
							uint8_t *our_ip, uint8_t *dst_mac);
							
// Check if this is a ping reply for our request						
bool icmp_is_echo_reply(ipv4_header_t *ip_header);

uint16_t icmp_get_reply_sequence(ipv4_header_t *ip_header);
uint16_t icmp_get_reply_data_len(ipv4_header_t *ip_header);

/**
 * @file icmp.h
 * @brief ICMP echo (ping) request/reply handling.
 */
#pragma once
#include <klib/stdint.h>
#include <klib/stdbool.h>
#include "protocols.h"

/** @brief ICMP header (8 bytes). */
typedef struct __attribute__((packed)) {
    uint8_t type;        /**< ICMP_ECHO_REQUEST, ICMP_ECHO_REPLY, ... */
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier; /**< Matches replies to the requesting program. */
    uint16_t sequence;
} icmp_packet_t;

#define ICMP_HEADER_SIZE 8

/* ICMP message types */
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

/* Parameters of our ping implementation */
#define OUR_PING_IDENTIFIER 0x4321
#define PING_COUNT 4
#define PING_TIMEOUT_MS 2000
#define PING_INTERVAL_MS 2000
#define PING_DATA_LENGTH 17

/** @brief True if the packet is an echo request addressed to @p our_ip. */
bool icmp_is_ping_request(ipv4_header_t *ip_header, uint8_t *our_ip);

/** @brief Sends an echo reply mirroring the given request frame. */
void icmp_send_ping_reply(uint8_t *packet_buffer, uint32_t packet_len,
                          uint8_t *our_mac, uint8_t *our_ip);

/** @brief Sends an echo request to @p target_ip. */
bool icmp_send_echo_request(uint8_t *target_ip, uint16_t sequence, uint8_t *our_mac,
                            uint8_t *our_ip, uint8_t *dst_mac);

/** @brief True if the packet is an echo reply to one of our requests
 *         (matching OUR_PING_IDENTIFIER). */
bool icmp_is_echo_reply(ipv4_header_t *ip_header);

/** @brief Extracts the sequence number from an echo reply. */
uint16_t icmp_get_reply_sequence(ipv4_header_t *ip_header);

/** @brief Returns the payload length of an echo reply. */
uint16_t icmp_get_reply_data_len(ipv4_header_t *ip_header);

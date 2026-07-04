/**
 * @file icmp.c
 * @brief ICMP echo request/reply (ping) implementation.
 */
#include "net/icmp.h"
#include "drivers/rtl8139/rtl8139.h"
#include "klib/string.h"
#include "klib/stdio.h"

static uint16_t calculate_checksum(uint16_t *data, uint32_t length) {
    uint32_t sum = 0;

    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
	
    if (length > 0) {
        sum += *(uint8_t*)data;
    }
	
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
	
    return ~sum;
}

// Check if this is a ping request for us
bool icmp_is_ping_request(ipv4_header_t *ip_header, uint8_t *our_ip) {
    if (ip_header->dst_ip[0] != our_ip[0] ||
        ip_header->dst_ip[1] != our_ip[1] ||
        ip_header->dst_ip[2] != our_ip[2] ||
        ip_header->dst_ip[3] != our_ip[3])
    {
        return false;
    }

    if (ip_header->protocol != ICMP_PROTOCOL) {
        return false;
    }

    uint8_t ip_header_length_size = (ip_header->version_hl & 0x0F) * 4;
    icmp_packet_t *icmp = (icmp_packet_t*)((uint8_t*)ip_header + ip_header_length_size);

    return (icmp->type == ICMP_ECHO_REQUEST && icmp->code == 0);
}

// Send ping reply
void icmp_send_ping_reply(uint8_t *packet_buffer, uint32_t packet_len,
                          uint8_t *our_mac, uint8_t *our_ip) {
    uint8_t *src_mac = &packet_buffer[6];
    ipv4_header_t *ip_request = (ipv4_header_t*)(&packet_buffer[ETHER_HEADER_SIZE]);
    uint8_t ip_header_length_size = (ip_request->version_hl & 0x0F) * 4;
    icmp_packet_t *icmp_request = (icmp_packet_t*)((uint8_t*)ip_request + ip_header_length_size);

    uint16_t ip_total_len = ntohs(ip_request->total_length);
    uint32_t icmp_total = ip_total_len - ip_header_length_size;
    uint32_t icmp_data_len = icmp_total - ICMP_HEADER_SIZE;

    uint8_t reply[MAX_ETHER_PACKET_SIZE];
    uint32_t reply_len = ETHER_HEADER_SIZE + IPV4_HEADER_SIZE + icmp_total;

    // ===== Ethernet Header =====
    ether_header_t *eth = (ether_header_t*)reply;
    memcpy(eth->ether_dst_mac, src_mac,  6);
    memcpy(eth->ether_src_mac, our_mac,  6);
    eth->ether_type[0] = 0x08;
    eth->ether_type[1] = 0x00;

    // ===== IP Header =====
    ipv4_header_t *ip = (ipv4_header_t*)(&reply[ETHER_HEADER_SIZE]);
    ip->version_hl = 0x45;
    ip->service_type = 0;
    ip->total_length = htons(IPV4_HEADER_SIZE + icmp_total);
    ip->identification = htons(0x1234);
    ip->flags_fragment = 0;
    ip->ttl = DEFAULT_TTL;
    ip->protocol = ICMP_PROTOCOL;
    ip->header_checksum = 0;
    memcpy(ip->src_ip, our_ip, 4);
    memcpy(ip->dst_ip, ip_request->src_ip, 4);
    ip->header_checksum = calculate_checksum((uint16_t*)ip, IPV4_HEADER_SIZE);

    // ===== ICMP Header + Data =====
    icmp_packet_t *icmp = (icmp_packet_t*)(&reply[ETHER_HEADER_SIZE + IPV4_HEADER_SIZE]);
    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = icmp_request->identifier;
    icmp->sequence = icmp_request->sequence;
    memcpy((uint8_t*)icmp + ICMP_HEADER_SIZE,
           (uint8_t*)icmp_request + ICMP_HEADER_SIZE,
           icmp_data_len);
    icmp->checksum = calculate_checksum((uint16_t*)icmp, icmp_total);

    rtl8139_send_packet(reply, reply_len);
}

// Send ping request
bool icmp_send_echo_request(uint8_t *target_ip, uint16_t sequence,
                             uint8_t *our_mac, uint8_t *our_ip,
                             uint8_t *dst_mac) {
    uint8_t ping_data[PING_DATA_LENGTH] = "Hello from myos!";
    uint32_t data_len = PING_DATA_LENGTH - 1;

    uint8_t  packet[MAX_ETHER_PACKET_SIZE];
    uint32_t packet_len = ETHER_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + data_len;

    if (packet_len > sizeof(packet)) {
	  return false;
	}

    // ===== Ethernet Header =====
    ether_header_t *eth = (ether_header_t*)packet;
    memcpy(eth->ether_dst_mac, dst_mac, 6);
    memcpy(eth->ether_src_mac, our_mac, 6);
    eth->ether_type[0] = 0x08;
    eth->ether_type[1] = 0x00;

    // ===== IPv4 Header =====
    ipv4_header_t *ip = (ipv4_header_t*)(&packet[ETHER_HEADER_SIZE]);
    ip->version_hl = 0x45;
    ip->service_type = 0;
    ip->total_length = htons(IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + data_len);
    ip->identification = htons(0x1234);
    ip->flags_fragment = 0;
    ip->ttl = DEFAULT_TTL;
    ip->protocol = ICMP_PROTOCOL;
    ip->header_checksum = 0;
    memcpy(ip->src_ip, our_ip, 4);
    memcpy(ip->dst_ip, target_ip, 4);
    ip->header_checksum = calculate_checksum((uint16_t*)ip, IPV4_HEADER_SIZE);

    // ===== ICMP Header + Data =====
    icmp_packet_t *icmp = (icmp_packet_t*)(&packet[ETHER_HEADER_SIZE + IPV4_HEADER_SIZE]);
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = htons(OUR_PING_IDENTIFIER);
    icmp->sequence = htons(sequence);
    memcpy((uint8_t*)icmp + ICMP_HEADER_SIZE, ping_data, data_len);
    icmp->checksum = calculate_checksum((uint16_t*)icmp, ICMP_HEADER_SIZE + data_len);

    rtl8139_send_packet(packet, packet_len);
    return true;
}

// Check if this is a ping reply for our request	
bool icmp_is_echo_reply(ipv4_header_t *ip_header) {
    if (ip_header->protocol != ICMP_PROTOCOL) {
	  return false;
	}

    uint8_t ip_header_length_size = (ip_header->version_hl & 0x0F) * 4;
    icmp_packet_t *icmp = (icmp_packet_t*)((uint8_t*)ip_header + ip_header_length_size);
	
    return (icmp->type == ICMP_ECHO_REPLY &&
            icmp->code == 0 &&
            ntohs(icmp->identifier) == OUR_PING_IDENTIFIER);
}

uint16_t icmp_get_reply_sequence(ipv4_header_t *ip_header) {
    uint8_t ip_header_length_size = (ip_header->version_hl & 0x0F) * 4;
    icmp_packet_t *icmp = (icmp_packet_t*)((uint8_t*)ip_header + ip_header_length_size);
    return ntohs(icmp->sequence);
}

uint16_t icmp_get_reply_data_len(ipv4_header_t *ip_header) {
    uint8_t ip_ip_header_length_size = (ip_header->version_hl & 0x0F) * 4;
	uint16_t ip_total = ntohs(ip_header->total_length);
	uint16_t icmp_total = ip_total - ip_ip_header_length_size;
    return icmp_total - ICMP_HEADER_SIZE;
}

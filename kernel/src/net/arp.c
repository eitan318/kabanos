#include "net/arp.h"
#include "drivers/rtl8139/rtl8139.h"
#include "klib/string.h"
#include "klib/stdio.h"

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
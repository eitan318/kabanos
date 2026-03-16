#include "protocols.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/times.h>
#include <sys/types.h>

typedef unsigned int myos_socklen_t;

#define AF_RAW 1
#define SOCK_RAW 1

#define PROTO_RAW 0x0000
#define PROTO_IPV4 0x0800
#define PROTO_ARP 0x0806

// sockaddr for raw layer-2 sockets (ll - link layer, sa - socket address)
typedef struct {
  uint16_t sa_family;   // AF_RAW
  uint16_t sa_protocol; // ethertype to send/filter (host byte order)
  uint8_t
      sa_mac[6]; // destination MAC (for sendto) / source MAC (from recvfrom)
} __attribute__((packed)) sockaddr_ll_t;

#define PING_COUNT 4
#define PING_DATA "Hello from myos!"
#define PING_DATA_LEN strlen(PING_DATA)

// Get current kernel tick count
static uint32_t get_ticks(void) {
  struct tms t;
  return (uint32_t)times(&t);
}

//  Extract kernel arrival timestamp from recvfrom's sockaddr
// The kernel stuffs the packet's arrival tick into sa_mac[2..5].
static uint32_t get_arrival_ticks(const sockaddr_ll_t *from) {
  return ((uint32_t)from->sa_mac[2] << 24) | ((uint32_t)from->sa_mac[3] << 16) |
         ((uint32_t)from->sa_mac[4] << 8) | ((uint32_t)from->sa_mac[5]);
}

// Checksum
static uint16_t checksum(const uint16_t *data, uint32_t length) {
  uint32_t sum = 0;

  while (length > 1) {
    sum += *data++;
    length -= 2;
  }

  if (length > 0) {
    sum += *(uint8_t *)data;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}

// IP string parser
static int parse_ip(const char *str, uint8_t *out) {
  int octet = 0, count = 0;
  for (int i = 0;; i++) {
    char c = str[i];
    if (c >= '0' && c <= '9') {
      octet = octet * 10 + (c - '0');
    } else if (c == '.' || c == '\0') {
      if (count >= 4 || octet > 255)
        return -1;

      out[count++] = (uint8_t)octet;
      octet = 0;

      if (c == '\0')
        break;
    } else {
      return -1;
    }
  }
  return (count == 4) ? 0 : -1;
}

// Build + send one ICMP echo request 
static void send_ping(int sock, uint8_t *dst_mac, uint8_t *our_mac,
                      uint8_t *our_ip, uint8_t *dst_ip, uint16_t seq) {
  uint8_t pkt[ETHER_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE +
              PING_DATA_LEN];
  memset(pkt, 0, sizeof(pkt));

  // ===== Ethernet Header =====
  ether_header_t *eth = (ether_header_t *)pkt;
  memcpy(eth->ether_dst_mac, dst_mac, 6);
  memcpy(eth->ether_src_mac, our_mac, 6);
  eth->ether_type[0] = 0x08;
  eth->ether_type[1] = 0x00;

  // ===== IPv4 Header =====
  ipv4_header_t *ip = (ipv4_header_t *)(&pkt[ETHER_HEADER_SIZE]);
  ip->version_hl = 0x45;
  ip->service_type = 0;
  ip->total_length = htons(IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + PING_DATA_LEN);
  ip->identification = htons(0x1234);
  ip->flags_fragment = 0;
  ip->ttl = DEFAULT_TTL;
  ip->protocol = ICMP_PROTOCOL;
  ip->header_checksum = 0;
  memcpy(ip->src_ip, our_ip, 4);
  memcpy(ip->dst_ip, dst_ip, 4);
  ip->header_checksum = checksum((uint16_t *)ip, IPV4_HEADER_SIZE);

  // ===== ICMP Header + Data =====
  icmp_packet_t *icmp =
      (icmp_packet_t *)(&pkt[ETHER_HEADER_SIZE + IPV4_HEADER_SIZE]);
  icmp->type = ICMP_ECHO_REQUEST;
  icmp->code = 0;
  icmp->checksum = 0;
  icmp->identifier = htons(OUR_IDENTIFIER);
  icmp->sequence = htons(seq);
  memcpy((uint8_t *)icmp + ICMP_HEADER_SIZE, PING_DATA, PING_DATA_LEN);
  icmp->checksum = checksum((uint16_t *)icmp, ICMP_HEADER_SIZE + PING_DATA_LEN);

  sockaddr_ll_t dst_addr = {AF_RAW, PROTO_IPV4, {0}};
  memcpy(dst_addr.sa_mac, dst_mac, 6);
  uint32_t pkt_len =
      ETHER_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE + PING_DATA_LEN;
  sendto(sock, pkt, pkt_len, 0, &dst_addr, sizeof(dst_addr));
}

// Wait for ICMP echo reply
// Returns RTT in ms on success, -1 on timeout.
// t_send is the tick recorded just before send_ping() was called.
// The kernel stamps each packet's arrival tick into from.sa_mac[2..5],
static long wait_reply(int sock, uint8_t *src_ip, uint16_t seq,
                       uint32_t t_send) {
  uint8_t rx[MAX_PACKET_SIZE];

    for (int retries = 0; retries < 64; retries++) {
        sockaddr_ll_t from;
        myos_socklen_t flen = sizeof(from);
        ssize_t n = recvfrom(sock, rx, sizeof(rx), 0, &from, &flen);

    if (n <= 0) {
      printf("Request timed out.\n");
      return -1;
    }

    if (n < (ETHER_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE))
      continue;

    uint16_t ether_type = ((uint16_t)rx[12] << 8) | rx[13];
    if (ether_type != PROTO_IPV4)
      continue;

    ipv4_header_t *ip_header = (ipv4_header_t *)(&rx[ETHER_HEADER_SIZE]);
    if (ip_header->protocol != ICMP_PROTOCOL)
      continue;

    if (memcmp(ip_header->src_ip, src_ip, 4) != 0)
      continue;

    uint8_t ip_header_length_size = (ip_header->version_hl & 0x0F) * 4;
    icmp_packet_t *icmp =
        (icmp_packet_t *)((uint8_t *)ip_header + ip_header_length_size);

    if (icmp->type != ICMP_ECHO_REPLY)
      continue;

    if (ntohs(icmp->identifier) != OUR_IDENTIFIER)
      continue;

    if (ntohs(icmp->sequence) != seq)
      continue;

    // RTT: use kernel arrival timestamp from sa_mac[2..5]
    uint32_t t_recv = get_arrival_ticks(&from);
    uint32_t rtt_ms = t_recv - t_send;

    uint16_t ip_total = ntohs(ip_header->total_length);
    uint16_t data_len = ip_total - ip_header_length_size - ICMP_HEADER_SIZE;
    printf("Reply from %d.%d.%d.%d: bytes=%d time=%dms TTL=%d\n", src_ip[0],
           src_ip[1], src_ip[2], src_ip[3], data_len, rtt_ms, ip_header->ttl);
    return (long)rtt_ms;
  }

  printf("Request timed out.\n");
  return -1;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: ping <ip>\n");
    printf("Example: ping 8.8.8.8\n");
    return 1;
  }

  uint8_t target_ip[4];
  if (parse_ip(argv[1], target_ip) < 0) {
    printf("ping: invalid address '%s'\n", argv[1]);
    return 1;
  }

  uint8_t our_ip[4] = {10, 0, 0, 100};
  uint8_t gw_ip[4] = {10, 0, 0, 1};
  uint8_t our_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

  printf("\nPinging %d.%d.%d.%d with %d bytes of data:\n", target_ip[0],
         target_ip[1], target_ip[2], target_ip[3], PING_DATA_LEN);

  int sock = socket(AF_RAW, SOCK_RAW, PROTO_RAW);
  if (sock < 0) {
    printf("ping: socket() failed\n");
    return 1;
  }

  uint8_t arp_target[4];
  int same_subnet = (target_ip[0] == our_ip[0] && target_ip[1] == our_ip[1] &&
                     target_ip[2] == our_ip[2]);
  memcpy(arp_target, same_subnet ? target_ip : gw_ip, 4);

  uint8_t dst_mac[6];
  if (arp_resolve(sock, arp_target, our_mac, our_ip, dst_mac) < 0) {
    printf("ping: ARP resolution failed\n");
    return 1;
  }

  int sent = 0;
  int received = 0;
  uint32_t rtt_min = 0xFFFFFFFF;
  uint32_t rtt_max = 0;
  uint32_t rtt_sum = 0;

  for (uint16_t seq = 1; seq <= PING_COUNT; seq++) {
    uint32_t t_send = get_ticks();
    send_ping(sock, dst_mac, our_mac, our_ip, target_ip, seq);
    sent++;

    long rtt = wait_reply(sock, target_ip, seq, t_send);
    if (rtt >= 0) {
      received++;
      uint32_t r = (uint32_t)rtt;
      if (r < rtt_min)
        rtt_min = r;
      if (r > rtt_max)
        rtt_max = r;

      rtt_sum += r;
    }
    if (seq < PING_COUNT)
      sleep(1);
  }

  printf("\nPing statistics for %d.%d.%d.%d:\n", target_ip[0], target_ip[1],
         target_ip[2], target_ip[3]);
  printf("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss),\n",
         sent, received, sent - received,
         sent ? (sent - received) * 100 / sent : 0);
  if (received > 0) {
    printf("Approximate round trip times in milli-seconds:\n");
    printf("    Minimum = %dms, Maximum = %dms, Average = %dms\n", rtt_min,
           rtt_max, rtt_sum / received);
  }
  printf("\n");

    uint8_t our_ip[4] = {10, 0, 0, 100};
    uint8_t gw_ip[4] = {10, 0, 0, 1};
    uint8_t our_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    printf("\nPinging %d.%d.%d.%d with %d bytes of data:\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3], PING_DATA_LEN);

    int sock = socket(AF_RAW, SOCK_RAW, PROTO_RAW);
    if (sock < 0) {
        printf("ping: socket() failed\n");
        return 1;
    }

    uint8_t arp_target[4];
    int same_subnet = (target_ip[0] == our_ip[0] &&
                       target_ip[1] == our_ip[1] &&
                       target_ip[2] == our_ip[2]);
    memcpy(arp_target, same_subnet ? target_ip : gw_ip, 4);

    uint8_t dst_mac[6];
    if (arp_resolve(arp_target, dst_mac) < 0) {
        printf("ping: ARP resolution failed\n");
        return 1;
    }
    
    int sent = 0;
    int received = 0;
    uint32_t rtt_min = 0xFFFFFFFF;
    uint32_t rtt_max = 0;
    uint32_t rtt_sum = 0;
    
    for (uint16_t seq = 1; seq <= PING_COUNT; seq++) {
        uint32_t t_send = get_ticks();
        send_ping(sock, dst_mac, our_mac, our_ip, target_ip, seq);
        sent++;

        long rtt = wait_reply(sock, target_ip, seq, t_send);
        if (rtt >= 0) {
            received++;
            uint32_t r = (uint32_t)rtt;
            if (r < rtt_min) 
                rtt_min = r;
            if (r > rtt_max) 
                rtt_max = r;

            rtt_sum += r;
        }
        if (seq < PING_COUNT)
            sleep(1);
    }

    printf("\nPing statistics for %d.%d.%d.%d:\n",
           target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
    printf("    Packets: Sent = %d, Received = %d, Lost = %d (%d%% loss),\n",
           sent, received, sent - received,
           sent ? (sent - received) * 100 / sent : 0);
    if (received > 0) {
        printf("Approximate round trip times in milli-seconds:\n");
        printf("    Minimum = %dms, Maximum = %dms, Average = %dms\n",
               rtt_min, rtt_max, rtt_sum / received);
    }
    printf("\n");

    _close(sock);
    return (received == sent) ? 0 : 1;
}

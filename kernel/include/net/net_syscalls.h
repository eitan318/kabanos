#pragma once
#include <klib/stdint.h>
#include <klib/stddef.h>
#include "arch/i686/timer.h"

// Address families
#define AF_RAW    1

// Socket types
#define SOCK_RAW  1

// Protocols (used in socket() and bind())
#define PROTO_RAW   0x0000   // receive everything
#define PROTO_IPV4  0x0800
#define PROTO_ARP   0x0806

// sockaddr for raw layer-2 sockets (ll - link layer, sa - socket address)
typedef struct {
    uint16_t sa_family;      // AF_RAW
    uint16_t sa_protocol;    // ethertype to send/filter (host byte order)
    uint8_t  sa_mac[6];      // destination MAC (for sendto) / source MAC (from recvfrom)
} __attribute__((packed)) sockaddr_ll_t;

typedef struct sockaddr {
    uint16_t sa_family;
    uint8_t  sa_data[14];
} sockaddr_t;

void net_socket_deliver(uint8_t *packet, uint32_t len);
long sys_socket(int domain, int type, int protocol);
long sys_bind(int fd, struct sockaddr *addr, uint32_t addrlen);
long sys_sendto(int fd, void *buf, size_t len, int flags,
                struct sockaddr *dest_addr, uint32_t addrlen);
long sys_recvfrom(int fd, void *buf, size_t len, int flags,
                  struct sockaddr *src_addr, uint32_t *addrlen);
long sys_arp_resolve(uint8_t *target_ip, uint8_t *out_mac);                  
long sys_net_close(int fd);
void sys_net_close_all(void);                 
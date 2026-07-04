/**
 * @file net_syscalls.h
 * @brief Raw-socket syscall interface.
 */
#pragma once
#include <klib/stdint.h>
#include <klib/stddef.h>
#include "arch/i686/timer.h"

/* Address families */
#define AF_RAW    1

/* Socket types */
#define SOCK_RAW  1

/* Protocols (used in socket() and bind()) */
#define PROTO_RAW   0x0000   /**< Receive everything. */
#define PROTO_IPV4  0x0800
#define PROTO_ARP   0x0806

/**
 * @brief Socket address for raw layer-2 sockets
 *        (ll = link layer, sa = socket address).
 */
typedef struct {
    uint16_t sa_family;   /**< AF_RAW. */
    uint16_t sa_protocol; /**< EtherType to send/filter (host byte order). */
    uint8_t  sa_mac[6];   /**< Destination MAC for sendto(); source MAC
                               from recvfrom(). */
} __attribute__((packed)) sockaddr_ll_t;

/** @brief Generic socket address (POSIX-shaped). */
typedef struct sockaddr {
    uint16_t sa_family;
    uint8_t  sa_data[14];
} sockaddr_t;

/**
 * @brief Delivers a received frame to every bound socket whose protocol
 *        filter matches. Called from the NIC receive path.
 */
void net_socket_deliver(uint8_t *packet, uint32_t len);

/** @brief Creates a raw socket. Returns an fd or negative errno. */
long sys_socket(int domain, int type, int protocol);

/** @brief Binds a socket to a protocol filter. */
long sys_bind(int fd, struct sockaddr *addr, uint32_t addrlen);

/** @brief Transmits a frame to the MAC given in @p dest_addr. */
long sys_sendto(int fd, void *buf, size_t len, int flags,
                struct sockaddr *dest_addr, uint32_t addrlen);

/** @brief Receives the next matching frame; blocks until one arrives. */
long sys_recvfrom(int fd, void *buf, size_t len, int flags,
                  struct sockaddr *src_addr, uint32_t *addrlen);

/** @brief Resolves an IP to a MAC address via ARP on behalf of userland. */
long sys_arp_resolve(uint8_t *target_ip, uint8_t *out_mac);

/** @brief Closes a socket fd. */
long sys_net_close(int fd);

/** @brief Closes all sockets (used at process exit). */
void sys_net_close_all(void);

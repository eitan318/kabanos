#include "net/net_syscalls.h"
#include "drivers/rtl8139/rtl8139.h"
#include "mm/kmalloc.h"
#include "klib/string.h"
#include "klib/stdio.h"
#include "arch/i686/errno.h"
#include "sched/sched.h"
#include "net/arp.h"
#include "net/icmp.h"
#include "net/protocols.h"

#define MAX_SOCKETS 16
#define SOCKET_RX_BUF_SIZE 2048
#define SOCKET_RX_QUEUE 4

typedef struct {
    uint8_t data[SOCKET_RX_BUF_SIZE];
    uint32_t len;
    uint32_t arrived_ticks; 
} rx_packet_t;

typedef struct {
    bool in_use;
    uint16_t bound_protocol;  // ethertype filter (0 = accept all)

    rx_packet_t rx_queue[SOCKET_RX_QUEUE];
    uint8_t rx_head;
    uint8_t rx_tail;
    uint8_t rx_count;
} socket_t;

#define SOCKET_FD_BASE 64

static socket_t g_sockets[MAX_SOCKETS];

static socket_t *fd_to_socket(int fd) {
    int idx = fd - SOCKET_FD_BASE;
    if (idx < 0 || idx >= MAX_SOCKETS)
        return NULL;
    if (!g_sockets[idx].in_use)        
        return NULL;
    return &g_sockets[idx];
}

static int arp_handle(uint8_t *packet, uint32_t len) {
    arp_packet_t *arp = (arp_packet_t*)(&packet[ETHER_HEADER_SIZE]);

    // auto-learn any ARP reply into the cache
    if (ntohs(arp->operation) == ARP_OP_REPLY)
        arp_cache_insert(arp->src_ip, arp->src_mac);

    if (check_arp_is_for_us(arp, rtl8139_get_ip())) {
        arp_send_reply(arp, rtl8139_get_mac(), rtl8139_get_ip());
        return 0;
    }
    return 1;
}

static int icmp_handle(uint8_t *packet, uint32_t len)
{
    ipv4_header_t *ip = (ipv4_header_t*)(&packet[ETHER_HEADER_SIZE]);
    if (icmp_is_ping_request(ip, rtl8139_get_ip())) {
        icmp_send_ping_reply(packet, len, rtl8139_get_mac(), rtl8139_get_ip());
    }

    return 1;
}

//  Called from rtl8139 ISR – deliver packet
void net_socket_deliver(uint8_t *packet, uint32_t len) {
    if (len < ETHER_HEADER_SIZE) 
        return;

    uint16_t ethertype = ((uint16_t)packet[12] << 8) | packet[13];

    // no need sockets, just send reply to the request
    if (!arp_handle(packet, len) || !icmp_handle(packet, len))
    {
        return;
    }

    for (int i = 0; i < MAX_SOCKETS; i++) {
        socket_t *s = &g_sockets[i];
        if (!s->in_use) 
            continue;

        if (s->bound_protocol != 0 && s->bound_protocol != ethertype)
            continue;

        if (s->rx_count >= SOCKET_RX_QUEUE) {
            kdebugf("net_socket_deliver: socket %d queue full, dropping\n", i);
            continue;
        }

        rx_packet_t *slot = &s->rx_queue[s->rx_tail];
        uint32_t copy = len < SOCKET_RX_BUF_SIZE ? len : SOCKET_RX_BUF_SIZE;
        memcpy(slot->data, packet, copy);
        slot->len = copy;
        slot->arrived_ticks = timer_get_ticks();

        s->rx_tail = (s->rx_tail + 1) % SOCKET_RX_QUEUE;
        s->rx_count++;
    }
}

//  sys_socket
long sys_socket(int domain, int type, int protocol) {
    if (domain != AF_RAW || type != SOCK_RAW) 
        return -EAFNOSUPPORT;

    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!g_sockets[i].in_use) {
            memset(&g_sockets[i], 0, sizeof(socket_t));
            g_sockets[i].in_use = true;
            g_sockets[i].bound_protocol = (uint16_t)protocol;
       
            return SOCKET_FD_BASE + i;
        }
    }

    return -ENFILE;
}

//  sys_bind
long sys_bind(int fd, struct sockaddr *addr, uint32_t addrlen) {
    socket_t *s = fd_to_socket(fd);
    if (!s) 
        return -EBADF;

    if (!addr || addrlen < sizeof(sockaddr_ll_t)) 
        return -EINVAL;

    sockaddr_ll_t *ll = (sockaddr_ll_t*)addr;
    s->bound_protocol = ll->sa_protocol;

    return 0;
}

//  sys_sendto
long sys_sendto(int fd, void *buf, size_t len, int flags,
                struct sockaddr *dest_addr, uint32_t addrlen) {
    socket_t *s = fd_to_socket(fd);
    if (!s) 
        return -EBADF;

    if (!buf || len == 0) 
        return -EINVAL;

    rtl8139_send_packet((uint8_t *)buf, (uint32_t)len);
    return (long)len;
}

//  sys_recvfrom
//
//  Blocks until a packet arrives OR the timeout
//  expires. Each yield is ~1 timer tick.
//  RECVFROM_TIMEOUT_TICKS controls how long one
//  recvfrom() call will wait before returning -1.
//
//  Set to 0 to block forever (original behaviour).
#define RECVFROM_TIMEOUT_TICKS 3000   // how much time it will wait for reply. 1000 = 1 second

long sys_recvfrom(int fd, void *buf, size_t len, int flags,
                  struct sockaddr *src_addr, uint32_t *addrlen) {
    socket_t *s = fd_to_socket(fd);
    if (!s) 
        return -EBADF;

    if (!buf || len == 0) 
        return -EINVAL;

    uint32_t start = timer_get_ticks();
    while (s->rx_count == 0) {
        uint32_t elapsed = timer_get_ticks() - start;
        if (elapsed >= RECVFROM_TIMEOUT_TICKS) {
            return -1;
        }
        sys_yield();
    }

    // Dequeue from head
    rx_packet_t *slot = &s->rx_queue[s->rx_head];
    uint32_t copy = slot->len < (uint32_t)len ? slot->len : (uint32_t)len;
    memcpy(buf, slot->data, copy);

    if (src_addr && addrlen && *addrlen >= sizeof(sockaddr_ll_t)) {
        sockaddr_ll_t *ll = (sockaddr_ll_t *)src_addr;
        ll->sa_family = AF_RAW;
        ll->sa_protocol = ((uint16_t)slot->data[12] << 8) | slot->data[13];
        memcpy(ll->sa_mac, slot->data + 6, 6);
        // Encode arrival timestamp into sa_mac[2..5]
        uint32_t ts = slot->arrived_ticks;
        ll->sa_mac[2] = (ts >> 24) & 0xFF;
        ll->sa_mac[3] = (ts >> 16) & 0xFF;
        ll->sa_mac[4] = (ts >>  8) & 0xFF;
        ll->sa_mac[5] = (ts) & 0xFF;
        *addrlen = sizeof(sockaddr_ll_t);
    }

    s->rx_head = (s->rx_head + 1) % SOCKET_RX_QUEUE;
    s->rx_count--;

    return (long)copy;
}

long sys_arp_resolve(uint8_t *target_ip, uint8_t *out_mac) {
    if (!target_ip || !out_mac)
        return -EINVAL;
    return arp_resolve(target_ip,
                       rtl8139_get_mac(),
                       rtl8139_get_ip(),
                       out_mac) ? 0 : -1;
}

//  sys_net_close  –  called by sys_close when
//  the fd belongs to a socket (fd >= SOCKET_FD_BASE)
long sys_net_close(int fd) {
    int idx = fd - SOCKET_FD_BASE;
    if (idx < 0 || idx >= MAX_SOCKETS) 
        return -EBADF;
    if (!g_sockets[idx].in_use)       
        return -EBADF;
    memset(&g_sockets[idx], 0, sizeof(socket_t));

    return 0;
}

// sys_net_close_all - called when process destroy making sure 
// that all of the sockets are closed from the current process
void sys_net_close_all(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_sockets[i].in_use) {
            kdebugf("sys_net_close_all: force-closing leaked socket fd=%d\n",
                    SOCKET_FD_BASE + i);
            memset(&g_sockets[i], 0, sizeof(socket_t));
        }
    }
}
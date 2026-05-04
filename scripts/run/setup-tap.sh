#!/bin/bash

# Simple TAP Setup with Internet Access (NAT)

TAP_INTERFACE="tap0"
TAP_IP="10.0.0.1"
SUBNET="10.0.0.0/24"

# Detect host interface
if grep -qi microsoft /proc/version; then
    HOST_INTERFACE="eth0"  # WSL2
else
    HOST_INTERFACE=$(ip route | grep default | awk '{print $5}' | head -n1)
fi

# Create TAP interface
ip link delete "$TAP_INTERFACE" 2>/dev/null
ip tuntap add dev "$TAP_INTERFACE" mode tap
ip link set dev "$TAP_INTERFACE" up
ip addr add $TAP_IP/24 dev "$TAP_INTERFACE"

# Enable IP forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward

# Clean up any existing rules first
iptables -t nat -D POSTROUTING -s $SUBNET -o $HOST_INTERFACE -j MASQUERADE 2>/dev/null
iptables -D FORWARD -i $TAP_INTERFACE -o $HOST_INTERFACE -j ACCEPT 2>/dev/null
iptables -D FORWARD -i $HOST_INTERFACE -o $TAP_INTERFACE -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null
iptables -D FORWARD -i $HOST_INTERFACE -o $TAP_INTERFACE -j ACCEPT 2>/dev/null

# NAT: masquerade outgoing traffic from VM
iptables -t nat -A POSTROUTING -s $SUBNET -o $HOST_INTERFACE -j MASQUERADE

# Forward: VM -> internet
iptables -A FORWARD -i $TAP_INTERFACE -o $HOST_INTERFACE -j ACCEPT

# Forward: internet -> VM
# NOTE: Must be ACCEPT unconditionally, not just RELATED,ESTABLISHED.
# Raw socket OS bypasses conntrack — replies arrive as NEW state and
# would be dropped by a state-based rule.
iptables -A FORWARD -i $HOST_INTERFACE -o $TAP_INTERFACE -j ACCEPT
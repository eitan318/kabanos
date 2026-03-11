#!/bin/bash

# Simple TAP Cleanup


TAP_INTERFACE="tap0"
SUBNET="10.0.0.0/24"

echo "Cleaning up TAP interface..."

# Detect host interface
if grep -qi microsoft /proc/version; then
    HOST_INTERFACE="eth0"
else
    HOST_INTERFACE=$(ip route | grep default | awk '{print $5}' | head -n1)
fi

# Remove iptables rules
if [ -n "$HOST_INTERFACE" ]; then
    iptables -t nat -D POSTROUTING -s $SUBNET -o $HOST_INTERFACE -j MASQUERADE 2>/dev/null
    iptables -D FORWARD -i $TAP_INTERFACE -o $HOST_INTERFACE -j ACCEPT 2>/dev/null
    iptables -D FORWARD -i $HOST_INTERFACE -o $TAP_INTERFACE -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null
fi

# Delete TAP interface
ip link delete "$TAP_INTERFACE" 2>/dev/null

echo "Cleanup complete!"
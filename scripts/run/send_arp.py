#!/usr/bin/env python3
"""
ARP Request Sender - Sends ARP requests to your custom OS

This script sends ARP "who-has" requests to your custom OS running
in QEMU to test the ARP implementation.

Requirements:
    sudo pip3 install scapy

Usage:
    sudo python3 send_arp.py
"""

from scapy.all import *
import sys
import time

# Configuration - MUST match your OS settings
TARGET_IP = "10.0.0.100"        # Your OS's IP address
TARGET_MAC = "52:54:00:12:34:56"  # RTL8139's MAC (set by QEMU)
INTERFACE = "tap0"                # TAP interface (or br0 if using bridge)

def send_arp_request():
    """Send an ARP request asking 'Who has TARGET_IP?'"""
    print("=" * 60)
    print("ARP Request Sender")
    print("=" * 60)
    print(f"Target IP:  {TARGET_IP}")
    print(f"Interface:  {INTERFACE}")
    print("=" * 60)
    
    # Build ARP request packet
    # Ethernet frame: broadcast to everyone (ff:ff:ff:ff:ff:ff)
    # ARP: asking "who has TARGET_IP? Tell me (sender)"
    arp_request = Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(
        op=1,                    # 1 = ARP request
        pdst=TARGET_IP,          # Target IP (who has this IP?)
        hwdst="00:00:00:00:00:00"  # Unknown MAC (we're asking for it)
    )
    
    print("\nSending ARP request...")
    print(f"  Who has {TARGET_IP}? Tell {arp_request[ARP].psrc}")
    print("\nPacket details:")
    arp_request.show()
    
    # Send packet and wait for response (timeout=2 seconds)
    print("\nWaiting for ARP reply...")
    answered, unanswered = srp(arp_request, iface=INTERFACE, timeout=2, verbose=0)
    
    if answered:
        print("\n✓ SUCCESS! Received ARP reply:")
        for sent, received in answered:
            print(f"\n  {received[ARP].psrc} is at {received[ARP].hwsrc}")
            print("\nFull reply packet:")
            received.show()
        return True
    else:
        print("\n✗ No reply received")
        print("\nPossible reasons:")
        print("  1. Your OS hasn't implemented ARP reply yet")
        print("  2. Network setup (TAP/bridge) isn't working")
        print("  3. Your OS isn't running")
        print("\nCheck your QEMU debug output for ARP request packet!")
        return False

def send_continuous_arp():
    """Send ARP requests continuously (for testing)"""
    print("\nContinuous ARP mode - Press Ctrl+C to stop")
    print("=" * 60)
    
    count = 0
    while True:
        count += 1
        print(f"\n[Request #{count}] Sending ARP request to {TARGET_IP}...")
        
        arp_request = Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(
            op=1,
            pdst=TARGET_IP,
            hwdst="00:00:00:00:00:00"
        )
        
        answered, _ = srp(arp_request, iface=INTERFACE, timeout=1, verbose=0)
        
        if answered:
            for sent, received in answered:
                print(f"  ✓ Reply: {received[ARP].psrc} is at {received[ARP].hwsrc}")
        else:
            print(f"  ✗ No reply")
        
        time.sleep(2)  # Wait 2 seconds between requests

def send_custom_packet():
    """Send a custom test packet (not ARP)"""
    print("\nSending custom test packet...")
    
    # Custom Ethernet frame with EtherType 0x9999
    custom_packet = Ether(dst=TARGET_MAC, type=0x9999) / Raw(load=b"Hello from Python!")
    
    print("\nPacket details:")
    custom_packet.show()
    
    sendp(custom_packet, iface=INTERFACE, verbose=1)
    print("✓ Custom packet sent!")

def main():
    # Check if running as root
    if os.geteuid() != 0:
        print("ERROR: This script must be run as root (sudo)")
        print("Usage: sudo python3 send_arp.py")
        sys.exit(1)
    
    # Check if scapy is installed
    try:
        import scapy
    except ImportError:
        print("ERROR: scapy is not installed")
        print("Install it with: sudo pip3 install scapy")
        sys.exit(1)
    
    # Check if interface exists
    try:
        get_if_hwaddr(INTERFACE)
    except:
        print(f"ERROR: Interface '{INTERFACE}' not found")
        print("\nAvailable interfaces:")
        print(get_if_list())
        print("\nMake sure you ran: sudo bash setup-tap.sh")
        sys.exit(1)
    
    print("\nWhat would you like to do?")
    print("1. Send single ARP request (recommended for first test)")
    print("2. Send continuous ARP requests (for testing)")
    print("3. Send custom test packet")
    
    try:
        choice = input("\nEnter choice (1-3): ").strip()
        
        if choice == "1":
            send_arp_request()
        elif choice == "2":
            send_continuous_arp()
        elif choice == "3":
            send_custom_packet()
        else:
            print("Invalid choice")
            sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nStopped by user")
        sys.exit(0)

if __name__ == "__main__":
    main()

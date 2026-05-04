#!/usr/bin/env python3
"""
ICMP (Ping) Test Script - Test your OS's ping implementation

This script sends ICMP Echo Request (ping) packets to your custom OS
and waits for Echo Reply responses.

Requirements:
    sudo pip3 install scapy

Usage:
    sudo python3 test_icmp.py
"""

from scapy.all import *
import sys
import time

# Configuration - MUST match your OS settings
TARGET_IP = "10.0.0.100"        # Your OS's IP address
INTERFACE = "tap0"              # TAP interface

def send_single_ping():
    """Send a single ping and wait for reply"""
    print("=" * 60)
    print("Single Ping Test")
    print("=" * 60)
    print(f"Target IP:  {TARGET_IP}")
    print(f"Interface:  {INTERFACE}")
    print("=" * 60)
    
    # Build ICMP Echo Request
    packet = IP(dst=TARGET_IP)/ICMP(type=8, code=0, id=0x1234, seq=1)/"Hello from Python!"
    
    print("\nSending ping...")
    print(f"  Type: Echo Request (8)")
    print(f"  Code: 0")
    print(f"  Identifier: 0x1234")
    print(f"  Sequence: 1")
    print(f"  Data: 'Hello from Python!'")
    
    # Send and wait for reply (timeout=2 seconds)
    print("\nWaiting for reply...")
    reply = sr1(packet, iface=INTERFACE, timeout=2, verbose=0)
    
    if reply:
        if reply.haslayer(ICMP):
            icmp_reply = reply[ICMP]
            
            if icmp_reply.type == 0:  # Echo Reply
                print("\n✓ SUCCESS! Received ICMP Echo Reply")
                print(f"  From: {reply[IP].src}")
                print(f"  Type: {icmp_reply.type} (Echo Reply)")
                print(f"  Code: {icmp_reply.code}")
                print(f"  Identifier: 0x{icmp_reply.id:04x}")
                print(f"  Sequence: {icmp_reply.seq}")
                
                # Calculate round-trip time
                if hasattr(reply, 'time') and hasattr(packet, 'sent_time'):
                    rtt = (reply.time - packet.sent_time) * 1000
                    print(f"  Round-trip time: {rtt:.2f} ms")
                
                # Show the data
                if Raw in reply:
                    data = reply[Raw].load
                    print(f"  Data: {data.decode('ascii', errors='ignore')}")
                
                print("\n✓ Your OS successfully responded to ping!")
                return True
            else:
                print(f"\n✗ Received ICMP type {icmp_reply.type} instead of Echo Reply (0)")
                return False
        else:
            print("\n✗ Received packet but no ICMP layer")
            return False
    else:
        print("\n✗ No reply received (timeout)")
        print("\nPossible reasons:")
        print("  1. Your OS hasn't implemented ICMP reply yet")
        print("  2. ICMP code has a bug")
        print("  3. Network setup (TAP/bridge) isn't working")
        print("  4. Your OS isn't running")
        print("\nCheck your QEMU debug output for:")
        print("  - 'PACKET RECEIVED' message")
        print("  - 'IPv4' EtherType")
        print("  - 'Sending Ping Reply' message")
        return False

def send_continuous_pings():
    """Send continuous pings (like 'ping' command)"""
    print("\n" + "=" * 60)
    print("Continuous Ping Mode - Press Ctrl+C to stop")
    print("=" * 60)
    print(f"PING {TARGET_IP}: 56 data bytes")
    print()
    
    sequence = 0
    sent = 0
    received = 0
    
    try:
        while True:
            sequence += 1
            sent += 1
            
            # Build packet
            packet = IP(dst=TARGET_IP)/ICMP(type=8, id=0x1234, seq=sequence)/("x" * 56)
            
            # Send and wait for reply
            start_time = time.time()
            reply = sr1(packet, iface=INTERFACE, timeout=1, verbose=0)
            
            if reply and reply.haslayer(ICMP) and reply[ICMP].type == 0:
                rtt = (time.time() - start_time) * 1000
                received += 1
                print(f"64 bytes from {reply[IP].src}: icmp_seq={sequence} ttl={reply[IP].ttl} time={rtt:.1f} ms")
            else:
                print(f"Request timeout for icmp_seq {sequence}")
            
            time.sleep(1)  # Wait 1 second between pings
            
    except KeyboardInterrupt:
        print("\n")
        print("--- {} ping statistics ---".format(TARGET_IP))
        print(f"{sent} packets transmitted, {received} packets received, {((sent-received)/sent*100):.1f}% packet loss")

def send_various_sizes():
    """Test with different packet sizes"""
    print("\n" + "=" * 60)
    print("Testing Different Packet Sizes")
    print("=" * 60)
    
    sizes = [0, 8, 32, 56, 100, 500, 1000, 1400]
    
    for size in sizes:
        print(f"\nTesting with {size} byte payload...")
        
        data = "x" * size
        packet = IP(dst=TARGET_IP)/ICMP(type=8, id=0x1234, seq=1)/data
        
        reply = sr1(packet, iface=INTERFACE, timeout=2, verbose=0)
        
        if reply and reply.haslayer(ICMP) and reply[ICMP].type == 0:
            print(f"  ✓ Success! Reply received for {size} bytes")
        else:
            print(f"  ✗ Failed! No reply for {size} bytes")

def test_malformed_packets():
    """Test with various ICMP types and edge cases"""
    print("\n" + "=" * 60)
    print("Testing Edge Cases")
    print("=" * 60)
    
    tests = [
        ("Normal ping", IP(dst=TARGET_IP)/ICMP(type=8, code=0)),
        ("Wrong code", IP(dst=TARGET_IP)/ICMP(type=8, code=1)),
        ("Echo Reply (should ignore)", IP(dst=TARGET_IP)/ICMP(type=0, code=0)),
        ("Timestamp Request", IP(dst=TARGET_IP)/ICMP(type=13, code=0)),
    ]
    
    for name, packet in tests:
        print(f"\n{name}...")
        reply = sr1(packet, iface=INTERFACE, timeout=1, verbose=0)
        
        if reply and reply.haslayer(ICMP):
            print(f"  Reply received: Type={reply[ICMP].type}")
        else:
            print(f"  No reply (expected for some tests)")

def test_from_phone():
    """Instructions for testing from phone"""
    print("\n" + "=" * 60)
    print("Testing from Your Phone")
    print("=" * 60)
    print()
    print("Your phone can ping your OS too!")
    print()
    print("Steps:")
    print(f"1. Connect your phone to the same WiFi/network")
    print(f"2. Install a ping app (or use terminal)")
    print(f"3. Ping: {TARGET_IP}")
    print(f"4. Your OS should respond!")
    print()
    print("Android apps: 'PingTools', 'Network Utilities'")
    print("iOS apps: 'Network Ping', 'iNetTools'")
    print()

def show_packet_structure():
    """Show what a ping packet looks like"""
    print("\n" + "=" * 60)
    print("Ping Packet Structure")
    print("=" * 60)
    
    packet = IP(dst=TARGET_IP)/ICMP(type=8, id=0x1234, seq=1)/"Test"
    
    print("\nLayers:")
    packet.show()
    
    print("\nHexdump:")
    hexdump(packet)

def main():
    # Check if running as root
    if os.geteuid() != 0:
        print("ERROR: This script must be run as root (sudo)")
        print("Usage: sudo python3 test_icmp.py")
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
    
    # Menu
    while True:
        print("\n" + "=" * 60)
        print("ICMP (Ping) Test Menu")
        print("=" * 60)
        print("1. Send single ping (recommended for first test)")
        print("2. Continuous ping (like 'ping' command)")
        print("3. Test different packet sizes")
        print("4. Test edge cases")
        print("5. Show packet structure")
        print("6. Instructions for testing from phone")
        print("7. Exit")
        print()
        
        try:
            choice = input("Enter choice (1-7): ").strip()
            
            if choice == "1":
                send_single_ping()
            elif choice == "2":
                send_continuous_pings()
            elif choice == "3":
                send_various_sizes()
            elif choice == "4":
                test_malformed_packets()
            elif choice == "5":
                show_packet_structure()
            elif choice == "6":
                test_from_phone()
            elif choice == "7":
                print("\nGoodbye!")
                sys.exit(0)
            else:
                print("Invalid choice")
                
        except KeyboardInterrupt:
            print("\n\nStopped by user")
            continue
        except Exception as e:
            print(f"\nError: {e}")
            import traceback
            traceback.print_exc()

if __name__ == "__main__":
    main()
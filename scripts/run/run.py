import subprocess
import re
import os
import sys
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument("--kernel", required=True, help="Path to kernel.elf")
parser.add_argument("--image", required=True, help="Path to os.img")
parser.add_argument("--is_debug", action="store_true", help="Enable debug mode")
args = parser.parse_args()

KERNEL_ELF = args.kernel
OS_IMG = args.image
is_debug = args.is_debug  # True if --is_debug was passed

# Setup TAP interface
script_dir   = os.path.dirname(os.path.abspath(__file__))
setup_script = os.path.join(script_dir, "setup-tap.sh")
subprocess.run(["sudo", "bash", setup_script], check=True)

# QEMU command
qemu_run_cmd = [
    "qemu-system-i386",
    "-serial", "stdio",
    "-drive", f"format=raw,file={OS_IMG}",
    "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no",
    "-device", "rtl8139,netdev=net0,mac=52:54:00:12:34:56",
]

qemu_debug_cmd = [
    "qemu-system-i386",
    "-serial", "stdio",
    "-drive", f"format=raw,file={OS_IMG}",
    "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no",
    "-device", "rtl8139,netdev=net0,mac=52:54:00:12:34:56",
    "-s",
    "-S",
]

# Regex for the stack backtrace
stack_regex = re.compile(r"STACK_OF_PANIC\[123\]:\s*((?:0x[0-9A-Fa-f]+\s*)+)")

# Regex for the faulting instruction
fault_regex = re.compile(r"FAULTING_INSTRUCTION_OF_PANIC\[123\]:\s*(0x[0-9A-Fa-f]+)")


proc = subprocess.Popen(
    qemu_debug_cmd if is_debug else qemu_run_cmd,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
)

if proc.stdout is None:
    raise RuntimeError("Failed to capture QEMU output")

fault_addr = None
stack_addrs = []

# Stream QEMU output live
for line in proc.stdout:
    print(line, end="")

    detect_panic = True
    if not detect_panic:
        continue

    # Extract faulting instruction
    fmatch = fault_regex.search(line)
    if fmatch:
        fault_addr = fmatch.group(1)

    # Extract stack backtrace
    smatch = stack_regex.search(line)
    if smatch:
        stack_addrs = smatch.group(1).split()

        # If panic is fully detected, resolve symbols
        if fault_addr and stack_addrs:
            print("\n--- Panic detected! Resolving addresses ---\n")

            if fault_addr:
                print(f"FAULT @ {fault_addr}:")
                subprocess.run(["addr2line", "-e", KERNEL_ELF, fault_addr])
                print()

            if stack_addrs:
                print("STACK BACKTRACE:")
                for addr in stack_addrs:
                    subprocess.run(["addr2line", "-e", KERNEL_ELF, addr])
                print()

            print("--- End of panic resolution ---")
            break

proc.wait()

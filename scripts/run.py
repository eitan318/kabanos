#!/usr/bin/env python3

import subprocess
import re
import sys
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument("--kernel", required=True, help="Path to kernel.elf")
parser.add_argument("--image", required=True, help="Path to os.img")
args = parser.parse_args()

KERNEL_ELF = args.kernel
OS_IMG = args.image

# QEMU command
qemu_cmd = [
    "qemu-system-i386",
    "-debugcon",
    "stdio",
    "-drive",
    f"format=raw,file={OS_IMG}",
]

# Regex for the stack backtrace
stack_regex = re.compile(r"STACK:\s*((?:0x[0-9A-Fa-f]+\s*)+)")

# Regex for the faulting instruction
fault_regex = re.compile(r"FAULTING_INSTRUCTION:\s*(0x[0-9A-Fa-f]+)")

proc = subprocess.Popen(
    qemu_cmd,
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

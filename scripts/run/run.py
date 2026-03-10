import subprocess
import re
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

# QEMU command
qemu_run_cmd = [
    "qemu-system-i386",
    #    "-debugcon",
    "-serial",
    "stdio",
    #    "-d",
    #    "int",
    "-drive",
    f"format=raw,file={OS_IMG}",
]

qemu_debug_cmd = [
    "qemu-system-i386",
    #    "-debugcon",
    "-serial",
    "stdio",
    "-drive",
    f"format=raw,file={OS_IMG}",
    "-s",
    "-S",
]

# Regex for the stack backtrace
stack_regex = re.compile(r"STACK_OF_PANIC:\s*((?:0x[0-9A-Fa-f]+\s*)+)")

# Regex for the faulting instruction
fault_regex = re.compile(r"FAULTING_INSTRUCTION_OF_PANIC:\s*(0x[0-9A-Fa-f]+)")


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

    # 1. Update fault_addr if found
    fmatch = fault_regex.search(line)
    if fmatch:
        fault_addr = fmatch.group(1)

    # 2. Update stack_addrs if found
    smatch = stack_regex.search(line)
    if smatch:
        stack_addrs = smatch.group(1).split()

    # 3. TRIGGER: Check if we have what we need
    # We trigger if we found the fault_addr, even if stack is empty
    # (provided we've seen the "STACK_OF_PANIC" header line)
    if fault_addr and "STACK_OF_PANIC:" in line:
        print("\n--- Panic detected! Resolving addresses ---\n")

        print(f"FAULT @ {fault_addr}:")
        subprocess.run(["addr2line", "-e", KERNEL_ELF, fault_addr])
        print()

        if stack_addrs:
            print("STACK BACKTRACE:")
            for addr in stack_addrs:
                subprocess.run(["addr2line", "-e", KERNEL_ELF, addr])
        else:
            print("STACK BACKTRACE: (Empty)")

        print("\n--- End of panic resolution ---")
        break

proc.wait()

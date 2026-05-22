import subprocess
import re
import os
import sys
import argparse
import platform

# Parse command-line arguments
parser = argparse.ArgumentParser()
parser.add_argument("--kernel", required=True, help="Path to kernel.elf")
parser.add_argument("--image", required=True, help="Path to os.img")
parser.add_argument("--is_debug", action="store_true", help="Enable debug mode")
args = parser.parse_args()

KERNEL_ELF = args.kernel
OS_IMG = args.image
is_debug = args.is_debug  # True if --is_debug was passed

IS_WINDOWS = platform.system() == "Windows"

# Setup TAP interface ONLY on Linux
if not IS_WINDOWS:
    print("[*] Host environment: Linux. Configuring virtual network TAP...")
    script_dir = os.path.dirname(os.path.abspath(__file__))
    setup_script = os.path.join(script_dir, "setup-tap.sh")
    subprocess.run(["sudo", "bash", setup_script], check=True)
else:
    print("[*] Host environment: Windows. Bypassing TAP setup, utilizing user-mode networking backend.")

# Configure cross-platform network flags
# Linux maps to the pre-configured host bridge tap interface
# Windows maps to standard non-privileged slirp loopback translation
if IS_WINDOWS:
    net_flags = ["-netdev", "user,id=net0", "-device", "rtl8139,netdev=net0,mac=52:54:00:12:34:56"]
else:
    net_flags = ["-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no", "-device", "rtl8139,netdev=net0,mac=52:54:00:12:34:56"]

# QEMU commands built dynamically based on environment
base_qemu_cmd = [
    "qemu-system-i386",
    "-serial",
    "stdio",
    "-drive",
    f"format=raw,file={OS_IMG}",
] + net_flags

qemu_run_cmd = base_qemu_cmd
qemu_debug_cmd = base_qemu_cmd + ["-s", "-S"]

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
    if fault_addr and "STACK_OF_PANIC:" in line:
        print("\n--- Panic detected! Resolving addresses ---\n")

        # Resolve via addr2line (Ensure the native binutils or cross-compiler tools are active)
        addr2line_cmd = "i686-elf-addr2line" if IS_WINDOWS else "addr2line"
        
        # Quick fallback fallback verification check for native Windows builds
        try:
            subprocess.run([addr2line_cmd, "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except FileNotFoundError:
            addr2line_cmd = "addr2line" # Try local system resolution fallback
            
        print(f"FAULT @ {fault_addr}:")
        subprocess.run([addr2line_cmd, "-e", KERNEL_ELF, fault_addr])
        print()

        if stack_addrs:
            print("STACK BACKTRACE:")
            for addr in stack_addrs:
                subprocess.run([addr2line_cmd, "-e", KERNEL_ELF, addr])
        else:
            print("STACK BACKTRACE: (Empty)")

        print("\n--- End of panic resolution ---")
        break

proc.wait()
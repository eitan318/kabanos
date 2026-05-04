# Kabanos 

A custom 32-bit operating system built from scratch for the i686 (x86) architecture. Implements a complete OS stack — bootloader, kernel, virtual file system, TCP/IP networking, process scheduler, and a userland with a shell and standard utilities.

https://github.com/user-attachments/assets/70a78642-8550-4154-b75a-6c4127ea9be4



---

## Features

- **Two-stage bootloader** — custom 512-byte boot sector + protected-mode stage-2 loader (Multiboot-compliant, GRUB-compatible)
- **Memory management** — physical frame allocator, virtual memory with per-process paging, and a slab allocator (`kmalloc`/`kfree`)
- **Process & thread management** — `fork`/`exec`/`exit`, preemptive scheduler, context switching, zombie/wait semantics
- **Virtual File System (VFS)** — generic VFS interface with **MyFS**, a custom inode-based filesystem supporting files, directories, and symlinks
- **Device drivers** — VGA text console, PS/2 keyboard, PCI bus enumeration, RTL8139 NIC
- **TCP/IP networking** — ARP, ICMP, IP, and TCP layers; testable via QEMU TAP interface
- **System calls** — 50+ syscalls via `int 0x80` / `SYSENTER` with full errno support
- **Userland** — Newlib-backed C standard library, 16+ UNIX-style utilities, `vi` editor, `ping`, and a **TinyCC compiler** (self-hosting C compilation on the OS)
- **Interrupt subsystem** — GDT, IDT, ISR handlers, PIC, and CPU exception handling

---

## Architecture Overview

```
kabanos/
├── bootloader/          # Stage-1 (ASM) + Stage-2 (C/ASM) bootloader
├── kernel/
│   └── src/
│       ├── arch/i686/   # CPU init, GDT, IDT, ISR, paging, context switch
│       ├── drivers/     # Block, console, keyboard, PCI, RTL8139
│       ├── fs/          # VFS layer + MyFS custom filesystem
│       ├── mm/          # PMM, VMM, slab allocator
│       ├── sched/       # Preemptive scheduler
│       ├── proc/        # Process management & syscalls
│       ├── net/         # TCP/IP stack
│       └── klib/        # Kernel utility library
├── userland/
│   └── user_src/        # Shell, ls, cp, mv, cat, vi, ping, tcc, ...
├── common/              # Code shared between bootloader and kernel
├── extern/
│   ├── newlib_myos/     # Newlib C library stubs for MyOS
│   └── i386-tcc/        # TinyCC cross-compiler binary
├── tools/mkfs_myfs/     # Utility to format MyFS volumes
├── scripts/             # Build, run (QEMU), debug (GDB), and network scripts
├── cmake/               # Toolchain, sysroot, disk image, and initrd CMake modules
├── Dockerfile           # Hermetic build environment (Ubuntu 22.04 + cross-toolchain)
└── Makefile             # Top-level convenience targets
```

---

## Prerequisites

The build environment is fully containerized. You only need:

- [Docker](https://www.docker.com/) — for building
- [QEMU](https://www.qemu.org/) (`qemu-system-i386`) — for running (can also run inside Docker)
- Python 3 — for the QEMU launcher script

---

## Getting Started

### 1. Build the Docker image

```bash
make setup
```

This builds the `myos_builder` Docker image containing the `i686-myos-gcc` cross-compiler (Binutils 2.41 + GCC 13.2.0), NASM, CMake, Newlib 2.5.0, and TinyCC.

### 2. Build the OS

```bash
make build
```

This runs CMake inside the container and produces:

| Artifact | Path |
|---|---|
| Kernel ELF | `build/kernel/kernel.elf` |
| Bootloader binary | `build/bootloader/bootloader.bin` |
| Bootable disk image | `build/os.img` |
| Initial RAM disk | `build/initrd.tar` |

### 3. Run in QEMU

```bash
make run
```

Launches the OS in QEMU with a serial console and TAP-based network interface.

### 4. Debug with GDB

```bash
make debug
```

Starts QEMU in debug mode (halted at startup) and attaches GDB with the provided scripts in `scripts/gdb/`.

### 5. Open a build shell

```bash
make shell
```

Drops into an interactive Docker shell with the full cross-compile environment available.

### 6. Generate API documentation

```bash
make docs
```

Runs Doxygen and opens `docs/html/index.html` in your browser.

### 7. Clean build artifacts

```bash
make clean
```

---

## Boot Configuration

`boot.cfg` controls the bootloader parameters:

```
kernel=/kernel.elf
initrd=/initrd.tar
cmdline=fs_name=myfs root_device=atap2 init_proc=/bin/init.elf
```

- `fs_name` — filesystem driver to mount root with (`myfs`)
- `root_device` — ATA device for the root partition
- `init_proc` — path to PID-1 (`init.elf`)

---

## Userland Programs

| Program | Description |
|---|---|
| `init.elf` | PID-1 init process |
| `sh.elf` | Interactive shell with background job support |
| `ls`, `cat`, `cp`, `mv`, `rm`, `mkdir`, `rmdir`, `touch` | POSIX file utilities |
| `vi.elf` | Text editor |
| `ping.elf` | ICMP ping |
| `clock.elf` | System clock |
| `neofetch.elf` | System info display |
| `tcc.elf` | **TinyCC** — C compiler running natively on MyOS |
| `man.elf` | Manual page viewer |
| `test_fs.elf` | Filesystem test suite |

---

## Building the Cross-Compiler Manually

If you want to build the toolchain outside of Docker, see [`docs/build_cross_compiler.sh`](docs/build_cross_compiler.sh) for step-by-step instructions to compile Binutils, GCC, and Newlib targeting `i686-myos`.

---

## Documentation

| File | Description |
|---|---|
| [`docs/tips.md`](docs/tips.md) | Development tips and OS architecture notes |
| [`docs/drivers.md`](docs/drivers.md) | Driver abstraction layer design |
| [`docs/multiboot.md`](docs/multiboot.md) | Multiboot specification overview |
| [`docs/compile_with_tcc.md`](docs/compile_with_tcc.md) | On-OS compilation with TinyCC |

Source code is documented with Doxygen comments. Run `make docs` to generate the full HTML reference.

---

## Project Status

Active development. Core subsystems (memory, processes, VFS, networking) are functional and bootable in QEMU. The self-hosting TinyCC compiler demonstrates a working userland execution environment.

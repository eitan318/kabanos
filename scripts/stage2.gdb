file build/out/bootloader/stage2/stage2.elf
target remote localhost:1234
set architecture i386
set disassembly-flavor intel
b start
c
lay src



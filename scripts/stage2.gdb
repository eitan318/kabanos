file build/out/bootloader/stage2/stage2.elf
add-symbol-file build/out/kernel/kernel.elf 0xC0101000
target remote localhost:1234
set architecture i386
set disassembly-flavor intel
b start
c







target remote localhost:1234
set disassembly-flavor intel
set substitute-path /project .

file build/bootloader/stage2/stage2.elf

add-symbol-file build/bootroot/kernel.elf 0xC0101000

set architecture i386

b start
c
lay sr







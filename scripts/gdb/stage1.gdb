target remote localhost:1234
set disassembly-flavor intel
set substitute-path /project .


set architecture i8086

symbol-file build/bootloader/stage2/stage2.elf

b *0x7C00
c

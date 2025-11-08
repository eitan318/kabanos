target remote localhost:1234
symbol-file build/out/bootloader/stage2/stage2.elf
set disassembly-flavor intel
b *0x7e00



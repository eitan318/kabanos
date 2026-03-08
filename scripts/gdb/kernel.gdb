target remote localhost:1234
set disassembly-flavor intel
set substitute-path /project .

set architecture i386
file build/bootroot/kernel.elf

source scripts/gdb/load_user_syms.py

b kmain 
continue
layout src

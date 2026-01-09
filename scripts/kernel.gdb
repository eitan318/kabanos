file build/out/kernel/kernel.elf
target remote localhost:1234
set architecture i386
set disassembly-flavor intel
b kmain 
c
lay src

# to use:
#  gdb -x scripts/gdb_kernel.gdb 



file build/out/kernel/kernel.elf
target remote localhost:1234
set architecture i386
b start
c
lay src
b isr.c:58

# to use:
#  gdb -x scripts/gdb_kernel.gdb 



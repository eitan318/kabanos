file build/out/kernel/kernel.elf
target remote localhost:1234
set architecture i386
set disassembly-flavor intel
source /home/magshimim/repos/1001_myos/scripts/load_user_syms.py

b kmain 
c
lay src


# to use:
#  gdb -x scripts/gdb_kernel.gdb 
# load-user-syms /home/magshimim/repos/1001_myos/BOOT


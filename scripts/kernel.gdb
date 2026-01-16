file build/out/kernel/kernel.elf
target remote localhost:1234
set architecture i386
set disassembly-flavor intel
b kmain 
b test_tasks
b preemptive_switch_isr_handler
b switch_to

c
lay src

# to use:
#  gdb -x scripts/gdb_kernel.gdb 



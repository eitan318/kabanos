IMPORTANT TODO
--------------

(gdb) p str
$4 = 0x804a000 <error: Cannot access memory at address 0x804a000>
(gdb)


Bug on yield in sched_tick after first timeslice of test_c.elf


(gdb) p $eps
$2 = void
(gdb)


esp            0xf1009eb0          0xf1009eb0



The vuluntary switch is not working properly, I need to have 
an if in sched to decide whether iret ot ret, I cannot do iret each time

Make 2 processes yield to eachother properly


Why fork doesnt return child proc to fork addr




thread_switch_to[voluntary] () at /home/magshimim/repos/1001_myos/src/kernel/arch/i686/context_
switch.asm:58
(gdb) x/1wx $esp
0xf1009e94:     0xf100bfb4
(gdb)


thread_switch_to () at /home/magshimim/repos/1001_myos/src/kernel/arch/i686/context_switch.asm:
12
(gdb) x/10wx $esp
0xf1009e8c:     0xc0109382      0xe0001fc4      0xf100bfb4      0x00052000
0xf1009e9c:     0xf100c000      0x00000175      0xe0000ed4      0x00000020
0xf1009eac:     0x00052000      0xc0109341
(gdb)

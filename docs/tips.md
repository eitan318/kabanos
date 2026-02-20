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

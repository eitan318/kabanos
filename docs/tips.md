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

-----------------------------------


Weird:

│    0x400300        push   eax                                                           │
│  > 0x400301        call   0x4002ce                                                      │
│    0x400306        add    esp,0x4                                                       │
│    0x400309        mov    eax,ds:0x4008f4                                               │
│    0x40030e        push   eax                                                           │
│    0x40030f        call   0x4002ce                                                      │
│    0x400314        add    esp,0x4                                                       │
│    0x400317        mov    eax,ds:0x4008f8                                               │
└─────────────────────────────────────────────────────────────────────────────────────────┘
remote Thread 1.1 In:                                                   L??   PC: 0x400301
(gdb) p/x $eax
$6 = 0x410940
(gdb) x/4 $esp
0xbfffefe8:     0xffffffff      0xffffffff      0xffffffff      0xffffffff
(gdb) si // doing push of  val 0x410940 when stack is 0xfffffff
0x00400301 in ?? ()
(gdb) x/4 $esp // stack did grow down for push, but value wasnt entered to the addr... still fff
0xbfffefe4:     0xffffffff      0xffffffff      0xffffffff      0xffffffff
(gdb)



as you can see, even though push was executed, the pushed vale is nowhere in stack.. why?


0xbfffefe4:     0xffffffff      0xffffffff      0xffffffff      0xffffffff
(gdb) set *(int*)$esp = 0x12345678
(gdb) x/wx $esp
0xbfffefe4:     0xffffffff
(gdb)

0xbfffef5c
no, the fault is caused becose of the fffffff vals on the stack, it tries to weite to them later so there is a pf, the problem is probably in the fffffff faild push

after AAAAAAAAAAAAAAAAAAAAAABBBB
and than when switching to CC



a Page fault is happening on write to addr 0x7

here is PF
┐
│  > 0x4002ce        push   ebp                        │
│    0x4002cf        mov    ebp,esp                    │
│    0x4002d1        mov    edx,DWORD PTR ds:0x400920  │
│    0x4002d7        mov    eax,DWORD PTR [ebp+0x8]    │
│    0x4002da        mov    DWORD PTR [eax+0x8],edx    │
│    0x4002dd        mov    eax,DWORD PTR [ebp+0x8]    │
│    0x4002e0        mov    ds:0x400920,eax            │
│    0x4002e5        nop                               │
│   N0x4002e6        pop    ebp                        │
│    0x4002e7        ret                               │
│    0x4002e8        push   ebp                        │


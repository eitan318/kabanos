debugging: 
disass page_table_destry


max(CPL, RPL) ≤ DPL
CPL - Current privlag level of CPU
RPL - Requested Privlladge Level
DPL - Descriptor Privlladge Level

errcode selector - 0x18
0000 0000 0001 1000

GDTD = 0
TI = GDT
RPL = 0
index = b11 = 3


    0x0010791d
    0x0000001b
    0x00000202
    0xbffff000
    0x00000023


taskB () at /home/magshimim/repos/1001_myos/src/kernel/process/schedualer.c:64
(gdb) lay asm
(gdb) x/16wx $esp
0xc0001000:     Cannot access memory at address 0xc0001000
(gdb)


before esp change:

(gdb) n
(gdb) x/16wx $esp
0x7a4c: 0x001083a4      0x00007b20      0x0007b000      0x00007a78
0x7a5c: 0x00108373      0x0007b000      0x00114220      0x00000000
0x7a6c: 0x00007b20      0x0007a003      0x00007aa4      0x00007ab8
0x7a7c: 0x00102490      0x00007ac4      0x0007a000      0x00000003
(gdb)

after esp change to process kernel stack:

(gdb) x/16wx $esp
0xc0103fc0:     0x00000000      0x00000000      0x00000000      0x00000000
0xc0103fd0:     0x00000000      0x00000000      0x00000000      0x00000000
0xc0103fe0:     0x00000010      0x0000002d      0x00000000      0x001083aa
0xc0103ff0:     0x0000001b      0x00000202      0xc0001000      0x00000023
(gdb)


after popa and pop ds and skip err and int:

(gdb) x/16wx $esp
0xc0103fec:     0x001083aa      0x0000001b      0x00000202      0xc0001000
0xc0103ffc:     0x00000023      Cannot access memory at address 0xc0104000
(gdb)

PROB:
  vmspace->pd = physical_access(pd_phys);
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);


│    0xc0107d5a <kernel_vmspace_creat+30>    call   0xc010796f <physical_access>          │
│    0xc0107d5f <kernel_vmspace_creat+35>    add    esp,0x10                              │
│    0xc0107d62 <kernel_vmspace_creat+38>    mov    edx,DWORD PTR [ebp+0x8]               │
│  > 0xc0107d65 <kernel_vmspace_creat+41>    mov    DWORD PTR [edx],eax                   │
│    0xc0107d67 <kernel_vmspace_creat+43>    mov    eax,DWORD PTR [ebp+0x8]               │
│    0xc0107d6a <kernel_vmspace_creat+46>    mov    edx,DWORD PTR [ebp-0xc]               │
│    0xc0107d6d <kernel_vmspace_creat+49>    mov    DWORD PTR [eax+0x4],edx               │
│    0xc0107d70 <kernel_vmspace_creat+52>    mov    eax,DWORD PTR [ebp+0x8]               │
│    0xc0107d73 <kernel_vmspace_creat+55>    mov    eax,DWORD PTR [eax]                   │



(gdb) p/x $edx
$2 = 0xc0149000
(gdb)

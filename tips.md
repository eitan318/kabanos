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

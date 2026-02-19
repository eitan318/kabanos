bits 32
section .text

extern main
global _start
extern _exit

_start:
    call main
    push eax
    call _exit
    
    .halt:
        hlt 
        jmp .halt


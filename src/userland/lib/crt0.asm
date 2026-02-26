bits 32
section .text

global _start        ; Standard entry point name
extern c_start       ; Renaming your C function to avoid naming conflict
extern exit

_start:
    xor ebp, ebp     ; Standard practice to clear EBP for the debugger
    call c_start

    push eax
    call exit

.halt:
    hlt 
    jmp .halt

[bits 32]
section .multiboot.text
global kernel_start 
extern bringup 
extern stack_top

kernel_start:
    ; 1. Fix the stack immediately - NO C PROLOGUE ALLOWED
    mov esp, stack_top

    ; 2. Push Multiboot values so C can find them (EAX = Magic, EBX = Info)
    push ebx
    push eax

    ; 3. Call the C function to do the "heavy lifting" of paging
    call bringup

    ; 4. If we return, something went wrong
    cli
.hang:
    hlt
    jmp .hang

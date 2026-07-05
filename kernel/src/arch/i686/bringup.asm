[bits 32]
KERNEL_BASE equ 0xC0000000

section .multiboot.text
global kernel_start
extern bringup
extern stack_top

kernel_start:
    ; 1. Fix the stack immediately - NO C PROLOGUE ALLOWED
    ; Paging is off, so both the read and the stack itself must use
    ; physical addresses: read stack_top's value via its physical
    ; address, then convert the virtual top to physical.
    mov esp, [stack_top - KERNEL_BASE]
    sub esp, KERNEL_BASE

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

bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global switch_to
switch_to:
    mov ebp, [esp + 4]      ; load PCB pointer into EAX
    mov ebx, [ebp + 8]     ; load PCB->cr3 into EBX
    mov cr3, ebx             ; switch page directory

    mov esp, [ebp + 16]      ; ESP = PCB->kernel_esp
    pop ds
    popa
    add esp, 8              ; skip error code + int number

    mov eax, [ebp + 12] ; ss 
    push eax
    mov eax, [ebp + 16]; kernel_esp
    push eax
    mov eax, [ebp + 20]; eflags
    push eax
    mov eax, [ebp + 24]; cs 
    push eax
    mov eax, [ebp + 28]; eip
    push eax
    iret



bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global switch_to
switch_to:
    mov eax, [esp + 4]      ; load PCB pointer into EAX
    mov ebx, [eax + 12]     ; load PCB->cr3 into EBX
    mov cr3, ebx             ; switch page directory

    mov esp, [eax + 4]      ; ESP = PCB->kernel_esp
    pop ds
    popa
    add esp, 8              ; skip error code + int number
    iret

o

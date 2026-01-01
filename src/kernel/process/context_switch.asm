bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global switch_to
switch_to:
    mov esp, [esp + 4]

    pop ds
    popa
    add esp, 8
    iret

bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global switch_to
switch_to:
    mov eax, [esp + 4]       ;EAX = (TCB*)
    mov ebx, [eax + 12]      ;EBX = TCB->cr3
    mov cr3, ebx             ; switch page directory

    mov esp, [eax + 4]      ; ESP = TCB->kernel_esp
    pop ds
    popa
    add esp, 8              ; skip error code + int number
    iret
;
; global user_stub
; user_stub:
;     mov ax, 0x23 ; GDT_USER_DS_SEL
;     mov ds, ax
;     mov es, ax
;     mov fs, ax
;     mov gs, ax
;
;     pop eax        ; eax = real entry point
;     jmp eax        ; or call eax
;
; global switch_debug
; switch_debug:
;     mov eax, [esp + 4]      ; load PCB pointer into EAX
;     mov ebx, [eax + 12]     ; load PCB->cr3 into EBX
;     mov cr3, ebx             ; switch page directory


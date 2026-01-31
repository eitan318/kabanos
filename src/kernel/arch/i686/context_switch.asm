bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global thread_switch_to
thread_switch_to:
    ; [esp + 4] : kernel_esp 
    ; [esp + 0] : Return Address 
    mov eax, [esp + 4]
    mov esp, eax  ; esp = kernel_esp 

    ; This was pushed by 'hal_build_initial_frame' and common interrupt stub

    pop ds
    pop es
    pop fs
    pop gs
    popa ; general purpuse regs
    add esp, 8 ;'int_no' and 'err_code'

    ; Pops EIP, CS, EFLAGS (and SS/ESP if returning to user mode)
    iret

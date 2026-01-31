bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global thread_switch_to
thread_switch_to:
    ; Standard C Calling Convention (cdecl):
    ; [esp + 8] : pd_phys
    ; [esp + 4] : kernel_esp 
    ; [esp + 0] : Return Address 

    ; 1. Switch Page Directory (CR3) before we lose access to this stack
    mov eax, [esp + 8]
    mov cr3, eax

    ; 2. Switch to the new thread's stack
    mov eax, [esp + 4]
    mov esp, eax  

    ; 3. Restore Segments (Order: DS, ES, FS, GS)
    pop ds
    pop es
    pop fs
    pop gs

    ; 4. Restore General Purpose Registers
    popa 

    ; 5. Clean up interrupt stub data (int_no and err_code)
    add esp, 8

    ; 6. The jump to the thread
    iret

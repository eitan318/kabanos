bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global thread_switch_to
thread_switch_to:
    mov edx, [esp + 4]    ; edx = new_thread->esp
    mov eax, [esp + 8]    ; eax = new_thread->cr3

    mov esp, edx          

    test eax, eax ; pd_phys as 0 is flag for not switching
    je .skip_vmspace_switch
    mov cr3, eax 
.skip_vmspace_switch:

    ; Restore Segments (Order: DS, ES, FS, GS)
    pop ds
    pop es
    pop fs
    pop gs
    ;
    ; 4. Restore General Purpose Registers
    popa 

    ; 5. Clean up interrupt stub data (int_no and err_code)
    add esp, 8

    ; 6. The jump to the thread
    iret

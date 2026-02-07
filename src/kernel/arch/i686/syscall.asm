; This address must match the value in MSR_IA32_SYSENTER_EIP
global sysenter_handler_entry
extern syscall_handler_entry 

 sysenter_handler_entry:
    ; 1. Build the "CPU" part of the frame (Simulate an interrupt)
    push 0x23           ; User SS
    push ecx            ; User ESP 
    pushf               ; EFLAGS
    push 0x1B           ; User CS
    push edx            ; User EIP (Return address)

    ; 2. Build the "Software" part of the frame (arch_regs)
    push 0              ; Dummy error code
    push 0              ; Dummy int num
    pusha               ; Save ALL GPRs (eax, ecx, edx, ebx, esp, ebp, esi, edi)
    push ds
    push es
    push fs
    push gs

    mov eax, esp        
    push eax            ; Pass pointer to arch_regs/frame
    call syscall_handler_entry
    add esp, 4

    ; 5. Restore and Exit
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8          ; Skip dummies
    
    ; Restore for sysexit
    pop edx             ; User EIP
    add esp, 4          ; Skip CS
    popfd               ; EFLAGS
    pop ecx             ; User ESP
    add esp, 4          ; Skip SS
    
    sti
    sysexit

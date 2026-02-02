; This address must match the value in MSR_IA32_SYSENTER_EIP
global _sysenter_handler_entry
extern syscall_dispatch

;   ECX: Ring 3 Stack pointer for SYSEXIT
;   EDX: Ring 3 Return address
;   EAX: syscall number
;   Args: EBX, [stack], [stack + 4], EDI, ESI, [stack + 8] 
_sysenter_handler_entry:
    ; Build trap Frame for sysenter
    push 0x23           ; User SS
    push ecx            ; User ESP 
    pushf               ; EFLAGS
    push 0x1B           ; User CS
    push edx            ; User EIP
    
    push dword [ecx + 8] ; Arg 6
    push edi             ; Arg 5
    push esi             ; Arg 4
    push dword [ecx + 4] ; Arg 3 
    push dword [ecx]     ; Arg 2
    push ebx             ; Arg 1
    push eax             ; Syscall Number

    mov eax, esp        ; Pointer to frame
    push eax
    call syscall_dispatch
    add esp, 4          ; Clean up syscall frame pointer

    ; Pop the args we pushed (7 dwords: num + 6 args)
    add esp, 28         

    ; Restore state for SYSEXIT
    pop edx             ; Restore User EIP into EDX
    add esp, 4          ; Skip CS
    popfd               ; Restore EFLAGS
    pop ecx             ; Restore User ESP into ECX
    add esp, 4          ; Skip SS

    sysexit

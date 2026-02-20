bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global thread_switch_to
thread_switch_to:
    ; 1. SAVE CURRENT CONTEXT (The "Out" Thread)
    ; Since we are in the kernel, we only NEED to save callee-saved regs
    ; if we are yielding. But if we want a unified switch:
    pusha               ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds
    push es
    push fs
    push gs

    ; Save the old stack pointer into the old_thread struct
    mov eax, [esp + 52] ; Offset to reach 'old_esp' param (adjust based on pushes)
    mov [eax], esp

    ; -------------------------------------------------------
    ; 2. LOAD NEW CONTEXT (The "In" Thread)
    mov edx, [esp + 56] ; edx = new_thread->arch->esp
    mov ecx, [esp + 60] ; ecx = new_thread->process->cr3 (or equivalent)

    mov esp, edx        ; THE SWITCH: We are now on the new thread's stack

    ; 3. SWITCH ADDRESS SPACE
    test ecx, ecx
    jz .skip_vmspace_switch
    mov eax, cr3
    cmp eax, ecx        ; Optimization: don't reload if it's the same CR3
    je .skip_vmspace_switch
    mov cr3, ecx
.skip_vmspace_switch:

    ; 4. RESTORE NEW CONTEXT
    pop gs
    pop fs
    pop es
    pop ds
    popa

     ; Peek at what's below esp.
     ; Interrupt frame has CS at esp+4. User CS = 0x1b, kernel CS = 0x08.
     ; If it looks like an iret frame, use iret. Otherwise ret.
     mov eax, [esp + 12]
     cmp eax, 0x08        ; kernel CS = voluntary yield, came via call
     je .voluntary
     cmp eax, 0x1b        ; user CS = preempted from userspace
     je .from_interrupt
        
     ; fallthrough = unknown, probably voluntary kernel thread
 .voluntary:
     ret
 .from_interrupt:
     add esp, 8

    ; the EIP, CS, EFLAGS, etc., required by IRET.
     iret



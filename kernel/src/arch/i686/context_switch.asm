bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
; also is called with ret addr push
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

     cmp dword [esp + 12], 0x1b        ; user CS = preempted from userspace
     je .from_interrupt

     ret ;ret to pushed ret addr

 .from_interrupt:
     add esp, 8
    ; the EIP, CS, EFLAGS, etc., required by IRET.
     iret


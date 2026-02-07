bits 32

global thread_switch_to

;
; This should match the interrupt.asm isr_common and arch_regs
;
; void thread_switch_to(void **old_esp, void *new_esp, uint32_t cr3)
thread_switch_to:
    ; --- 1. SAVE CURRENT STATE ---
    pushfd          
    push cs         
    push .restore   
    push 0          ; Dummy error code
    push 0          ; Dummy interrupt number
    pusha           
    push ds         
    push es
    push fs
    push gs

    ; After 16 pushes (64 bytes), our arguments are at:
    ; [esp + 64] = Return address (where C called this asm)
    ; [esp + 68] = old_esp (void**)
    ; [esp + 72] = new_esp (void*)
    ; [esp + 76] = cr3     (uint32_t)

    mov eax, [esp + 68]
    mov [eax], esp     

    ; --- 2. LOAD NEW STATE ---
    mov eax, [esp + 76] ; Load new CR3
    mov edx, [esp + 72] ; Load new ESP

    mov cr3, eax        ; Switch page directory
    mov esp, edx        ; SWITCH STACKS HERE

    ; --- 3. RESTORE ---
    pop gs
    pop fs
    pop es
    pop ds
    popa 
    add esp, 8          ; Skip dummy int/err
    iret                ; Jumps to .restore (for kernel) or entry (for user)

.restore:
    ret                 ; Returns to the C function that called switch_to


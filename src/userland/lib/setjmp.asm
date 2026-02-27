[bits 32]
global setjmp
setjmp:
    mov eax, [esp + 4]    ; Get jmp_buf address
    mov [eax], ebp        ; Save registers...
    mov [eax + 4], ebx
    mov [eax + 8], edi
    mov [eax + 12], esi
    lea edx, [esp + 4]    ; Save stack pointer as it was before call
    mov [eax + 16], edx
    mov edx, [esp]        ; Save return address (EIP)
    mov [eax + 20], edx
    xor eax, eax          ; Return 0
    ret

global longjmp
longjmp:
    mov edx, [esp + 4]    ; jmp_buf address
    mov eax, [esp + 8]    ; value to return
    test eax, eax
    jnz .not_zero
    inc eax               ; longjmp cannot return 0
.not_zero:
    mov ebp, [edx]        ; Restore registers
    mov ebx, [edx + 4]
    mov edi, [edx + 8]
    mov esi, [edx + 12]
    mov esp, [edx + 16]
    jmp [edx + 20]        ; Jump back to saved EIP

bits 32

global div64_32
;
; void div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotient_out, uint32_t* remainder_out)
; using long divison
;
div64_32:
    push ebp
    mov ebp, esp
    push ebx
    
    ; Divide high 32 bits
    mov eax, [ebp + 12]  ; dividend HIGH
    xor edx, edx
    mov ecx, [ebp + 16]  ; divisor
    div ecx              ; eax = quotient_high, edx = remainder
    
    mov ebx, [ebp + 20]  ; quotient_out
    mov [ebx + 4], eax   ; store quotient HIGH
    
    push edx             ; save remainder on stack instead of ESI
    
    ; Divide low 32 bits
    mov eax, [ebp + 8]   ; dividend LOW
    pop edx              ; restore remainder
    div ecx              ; eax = quotient_low, edx = final remainder
    
    mov [ebx], eax       ; store quotient LOW
    
    mov ebx, [ebp + 24]  ; remainder_out
    mov [ebx], edx       ; store remainder
    
    pop ebx
    pop ebp
    ret

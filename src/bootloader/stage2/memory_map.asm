[BITS 32]
global x86_e820_get_next_block 

%include "mode_switch.inc"

e820_signature   equ 0x534D4150
;
; int ASMCALL x86_E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);
;
x86_e820_get_next_block:

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_enter_real_mode

    ; save modified regs
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ds
    push es

    ; setup params
    linear_to_segment_offset [bp + 8], es, edi, di     ; es:di pointer to structure
    
    linear_to_segment_offset [bp + 12], ds, esi, si    ; ebx - pointer to continuationId
    mov ebx, ds:[si]

    mov eax, 0xE820                             ; eax - function
    mov edx, e820_signature                      ; edx - signature
    mov ecx, 24                                 ; ecx - size of structure

    ; call interrupt
    int 0x15

    ; test results
    cmp eax,e820_signature 
    jne .error

    .if_success:
        mov eax, ecx            ; return size
        mov ds:[si], ebx        ; fill continuation parameter
        jmp .endif

    .error:
        mov eax, -1

    .endif:

    ; restore regs
    pop es
    pop ds
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx

    push eax

    x86_enter_protected_mode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

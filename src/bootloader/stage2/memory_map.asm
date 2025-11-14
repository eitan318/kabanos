; 32-bit cdecl:
; extern bool bios_fill_memory_map(E820Entry *buf, int max_entries);
;   [esp+4]  = pointer to buffer (linear)
;   [esp+8]  = max entries
[BITS 32]
global bios_fill_memory_map
%include "mode_switch.inc"

bios_fill_memory_map:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov esi, [ebp+12]    ; max entries

    ; switch to real mode
    x86_enter_real_mode

[BITS 16]

    linear_to_segment_offset [ebp+8], es, edi, di

    xor ebx, ebx          ; continuation = 0
    mov ecx, 20h          ; size of entry = 32 bytes
    mov edx, 0x534D4150   ; "SMAP"

.next:
    mov eax, 0xE820
    int 0x15
    xor eax, eax  ; assume failiur
    jc .done              ; CF=1 → stop
    mov eax, 1 ; success

    test ebx, ebx
    jz .done              ; no more entries

    add di, 20h           ; next entry
    dec si
    jnz .next


.done:
    push eax
    ; back to protected mode
    x86_enter_protected_mode

[BITS 32]
    pop eax

    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret


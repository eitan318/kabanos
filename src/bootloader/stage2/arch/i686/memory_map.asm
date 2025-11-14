; ============================================================
; bios_fill_memory_map(e820_entry_t* buf, int max_entries)
; ============================================================
; 32-bit cdecl:
;   [esp+4]  = pointer to buffer (linear)
;   [esp+8]  = max entries
;
; Each entry is 20 bytes (0x14)
;
[BITS 32]
global bios_fill_memory_map
%include "mode_switch.inc"

bios_fill_memory_map:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov edi, [ebp+8]     ; pointer to buffer
    mov esi, [ebp+12]    ; max entries
    mov edx, edi         ; save start for ES:DI conversion later

    ; switch to real mode
    x86_enter_real_mode

[BITS 16]

    ; Convert protected-mode pointer → segment:offset
    ; real-mode ES:DI must point to memory <1MB
    ; you MUST identity-map the first MB in paging
    mov ax, dx
    shr dx, 4
    and ax, 0xF
    mov es, dx
    mov di, ax

    xor ebx, ebx          ; continuation = 0
    mov ecx, 20h          ; size of entry = 32 bytes
    mov edx, 0x534D4150   ; "SMAP"

.next:
    mov eax, 0xE820
    int 0x15
    xor eax, eax  ; assume failiur
    jc .done              ; CF=1 → stop

    test ebx, ebx
    jz .done              ; no more entries

    add di, 20h           ; next entry
    dec si
    jnz .next

    mov eax, 1
    push eax

.done:
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


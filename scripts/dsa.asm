[bits 16]

global bios_check_lba_support
global bios_read_lba
global bios_read_chs
global bios_get_drive_params

%include "mode_switch.inc"

section .text


;
; bool bios_get_drive_params(uint8_t drive, uint8_t* heads, uint8_t* sectors, uint16_t* cylinders);
;
; Returns true (1) on success, false (0) on failure
;

bios_get_drive_params:
    [bits 32]
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    ; Arguments (cdecl 32-bit):
    ; [ebp+8]  = uint8_t drive
    ; [ebp+12] = uint8_t* heads
    ; [ebp+16] = uint8_t* sectors
    ; [ebp+20] = uint16_t* cylinders

    mov dl, [ebp+8]

    x86_enter_real_mode
    [bits 16]

    mov ah, 0x08
    int 0x13               ; Get drive parameters

    jc .fail               ; CF set → failure

    ; on success:
    ; CH = low 8 bits of cylinders
    ; CL bits 6-7 = high 2 bits of cylinders
    ; CL bits 0–5 = sectors per track
    ; DH = max head number (0-based)

    ; heads = DH + 1
    movzx bx, dh
    inc bx
    linear_to_segment_offset [bp + 12], es, esi, si
    mov [es:si], bl

    ; sectors = CL & 3Fh
    mov bl, cl
    and bl, 0x3F
    linear_to_segment_offset [bp + 16], es, esi, si
    mov [es:si], bl

    ; cylinders = ((CL & 0xC0) << 2) | CH + 1
    movzx bx, ch
    mov ah, cl
    and ah, 0xC0
    shl ah, 2
    or  bh, ah
    inc bx

    linear_to_segment_offset [bp + 20], es, esi, si
    mov [es:si], bx

    mov eax, 1              ; success
    jmp .done

.fail:
    xor eax, eax            ; failure

.done:
    push eax

    x86_enter_protected_mode
    [bits 32]
    pop eax
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret

    
; 
; Check if INT 13h Extensions (LBA) are supported
; Sig: extern bool bios_check_lba_support(uint8_t disk_number);
;
bios_check_lba_support:
    [bits 32]
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi

    x86_enter_real_mode
    [bits 16]

    mov dl, [bp + 8]        ; drive number
    mov ah, 0x41
    mov bx, 0x55AA
    stc
    int 0x13

    xor eax, eax            ; assume failure
    jc .done
    cmp bx, 0xAA55
    jne .done
    mov eax, 1              ; success

.done:
    x86_enter_protected_mode
    [bits 32]

    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret


;
; Load sectors using LBA (INT 13h AH=42h)
; Sig: bool bios_read_lba(uint8_t disk_number, struct DiskAddressPacket* dap);
;
bios_read_lba:
    [bits 32]
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi

    ; Get parameters from stack (cdecl)
    mov dl, [ebp + 8]       ; disk_number
    mov esi, [ebp + 12]     ; pointer to DAP (32-bit)

    ; Enter real mode
    x86_enter_real_mode
    [bits 16]

    ; Convert linear address to segment:offset
    mov si, si              ; Keep lower 16 bits in SI
    shr esi, 4
    mov ds, si              ; Segment in DS
    mov si, [ebp + 12]      ; Reload full address
    and si, 0x0F            ; Offset within segment

    mov ah, 0x42            ; INT 13h LBA read
    stc    
    int 0x13

    ; Set return value: 1 on success, 0 on failure
    mov eax, 1
    sbb eax, 0

.done:
    ; Return
    push eax

    x86_enter_protected_mode
    [bits 32]
    pop eax
    ; Cleanup stack
    pop esi
    pop edi
    pop ebx
    mov esp, ebp
    pop ebp
    ret

;
; Load sectors using CHS (INT 13h AH=02h)
; Sig: bool bios_read_chs(uint8_t drive_number, uint16_t cylinder, 
;                         uint16_t head, uint16_t sector,
;                         uint16_t count, void* dest);
;
bios_read_chs:
    [bits 32]
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    ; Get parameters
    mov dl, [ebp + 8]       ; drive number
    movzx ebx, word [ebp + 12]  ; cylinder (needs full word)
    mov dh, [ebp + 16]      ; head
    movzx ecx, byte [ebp + 20]  ; sector (1-based)
    movzx eax, word [ebp + 24]  ; count
    mov edi, [ebp + 28]     ; destination
    
    ; Build CX register for BIOS
    ; CH = lower 8 bits of cylinder
    ; CL = sector (bits 0-5) + upper 2 bits of cylinder (bits 6-7)
    mov ch, bl              ; Lower 8 bits of cylinder
    and cl, 0x3F            ; Keep sector bits 0-5
    mov si, bx
    shr si, 8
    and si, 0x03            ; Upper 2 bits of cylinder
    shl si, 6
    or cx, si              ; Combine into CL
    
    x86_enter_real_mode
    [bits 16]
    
    ; Setup buffer address (segment:offset)
    push edi
    shr edi, 4
    mov es, di              ; Segment
    pop edi
    and di, 0x0F            ; Offset within segment
    mov bx, di              ; ES:BX points to buffer
    
    ; Perform CHS read
    mov ah, 0x02            ; Read sectors
    stc
    int 0x13
    
    ; Set return value: 1 on success, 0 on failure
    mov eax, 1
    sbb eax, 0
    
.done:
    push eax
    
    x86_enter_protected_mode
    [bits 32]
    
    pop eax
    pop edi
    pop esi
    pop ebx
    mov esp, ebp
    pop ebp
    ret

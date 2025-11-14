[bits 16]

global bios_check_lba_support
global bios_read_lba
global bios_read_chs
global bios_get_drive_params
global bios_disk_reset

%include "mode_switch.inc"

section .text

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

    push dword 0  ; assum failiur
    x86_enter_real_mode
    [bits 16]

    mov dl, [bp + 8]        ; drive number
    mov ah, 0x41
    mov bx, 0x55AA
    stc
    int 0x13

    jc .done
    cmp bx, 0xAA55
    jne .done
    mov [esp], dword 1 ; sign success
   
.done:
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
;bios_get_drive_params(uint8_t drive, uint8_t* drive_type, uint8_t* HDDs_count,
;                      uint16_t* cylinders, uint16_t* heads, uint16_t* sectors);
;
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

    mov dl, [ebp+8]

    push dword 0  ; assum failiur
    x86_enter_real_mode
    [bits 16]

    mov ah, 0x08
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 0x13               ; Get drive parameters
    jc .done

    mov [esp], dword 1 ;sign success

    ; drive type from bl
    linear_to_segment_offset [bp + 12], es, esi, si
    mov [es:si], bl

    ; num of HDDs drive type from dl
    linear_to_segment_offset [bp + 16], es, esi, si
    mov [es:si], dl
 
    ; cylinders
    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    inc bx   
    linear_to_segment_offset [bp + 20], es, esi, si
    mov [es:si], bx

    ; heads from dh
    movzx ax, dh
    inc ax                       ; heads count = max index + 1
    linear_to_segment_offset [bp + 24], es, esi, si
    mov [es:si], ax

    ; sectors per track (bits 0–5 of CL)
    movzx ax, cl
    and ax, 0x3F
    linear_to_segment_offset [bp + 28], es, esi, si
    mov [es:si], ax

.done:

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
; Load sectors using LBA (INT 13h AH=42h)
; Sig: bool bios_read_lba(uint8_t disk_number, struct DiskAddressPacket* dap);
;
bios_read_lba:
    push ebp
    mov ebp, esp
    push ebx
    push edi
    push esi

    ; Get parameters from stack (cdecl)
    mov dl, [ebp + 8]    ; disk_number
    mov si, [ebp + 12]   ; pointer to DAP

    ; Enter real mode

    x86_enter_real_mode
    [bits 16]

    mov ah, 0x42          ; INT 13h LBA read
    stc    
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    int 0x13

    ; out params
    mov eax, 1
    sbb eax, 0

.done:
    ;return
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
    mov  ebp, esp

    x86_enter_real_mode
     ; save modified regs
    push ebx
    push es

    ; setup args
    mov dl, [bp + 8]    ; dl - drive

    mov ch, [bp + 12]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 13]    ; cl - cylinder to bits 6-7
    shl cl, 6

    mov dh, [bp + 16]   ; dh - head
    
    mov al, [bp + 20]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al


    mov al, [bp + 24]   ; al - count

    linear_to_segment_offset [bp + 28], es, ebx, bx

    ; call int13h
    mov ah, 02h
    stc
    int 13h


    ; set return value
    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    ; restore regs
    pop es
    pop ebx

    push eax
    x86_enter_protected_mode
    [bits 32]

    pop eax
   
    mov esp, ebp
    pop ebp
    ret


bios_disk_reset:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame


    x86_enter_real_mode 

    mov ah, 0
    mov dl, [bp + 8]    ; dl - drive
    stc
    int 13h

    mov eax, 1
    sbb eax, 0           ; 1 on success, 0 on fail   

    push eax

    x86_enter_protected_mode

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

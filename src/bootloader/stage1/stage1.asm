; stage1.asm - MBR bootloader
bits 16
STAGE1_ADDR         equ  0x7C00
STAGE2_ADDR         equ  0x7E00
STAGE2_LBA          equ  1

org STAGE1_ADDR

; Jump over BPB/EBR
jmp short stage1_start
nop

; BPB + EBR (59 bytes) - will be filled by mkfs.fat
times 59 db 0

; Include auto-generated stage2 size AFTER the BPB area
%include "stage2_sectors.inc"

global stage1_start
stage1_start:
    cli
    
    ; Save boot drive
    mov [boot_drive], dl
    
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, STAGE1_ADDR
    
    sti

    ; Load stage2 using BIOS INT 13h Extended Read
    mov ah, 0x42                ; Extended read function
    mov dl, [boot_drive]        ; Drive number
    mov si, dap                 ; Disk Address Packet
    int 0x13
    
    jc disk_error

    ; Jump to stage2
    mov dl, [boot_drive]        ; Pass boot drive to stage2
    jmp STAGE2_ADDR

disk_error:
    mov si, msg_disk_error
    call print_string
    
.hang:
    cli
    hlt
    jmp .hang

; Print string (SI = string pointer)
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

; Data
msg_disk_error: db 'Disk Error!', 0x0D, 0x0A, 0
boot_drive: db 0

; Disk Address Packet (DAP)
align 4
dap:
    db 0x10                     ; Size of DAP (16 bytes)
    db 0                        ; Reserved
    dw STAGE2_SECTORS_TOTAL           ; Number of sectors to read (from generated header)
    dw STAGE2_ADDR              ; Offset to load to
    dw 0                        ; Segment to load to
    dd STAGE2_LBA               ; Starting LBA (low 32 bits)
    dd 0                        ; Starting LBA (high 32 bits)

; Boot signature
times 510-($-$$) db 0
dw 0xAA55

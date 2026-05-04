; entry.asm - Stage2 entry point (16-bit to 32-bit transition)
bits 16

section .entry

extern __bss_start
extern __end
extern start

global g_boot_drive
global stage2_entry

; 
; Stage2 Entry (16-bit -> 32-bit transition)
; 

; --- Segment selectors ---
CODE_SEG_32b       equ 0x08      ; 32-bit code segment selector
DATA_SEG_32b       equ 0x10      ; 32-bit data segment selector
CODE_SEG_16b       equ 0x18      ; 16-bit code segment selector
DATA_SEG_16b       equ 0x20      ; 16-bit data segment selector

; --- Stack setup ---
STACK_16_END   equ 0xFFF0    ; Temporary 16-bit stack (real mode)

; --- CR0 flags ---
CR0_PE         equ 0x01      ; Protection Enable bit (bit 0)

stage2_entry:
    cli
    
    ; Save boot drive passed from stage1
    mov [g_boot_drive], dl

    ; Setup stack in 16-bit mode
    mov ax, ds
    mov ss, ax
    mov sp, STACK_16_END
    mov bp, sp

    ; Enable A20 gate
    call a20_enable
    
    ; Load GDT
    call gdt_load

    ; Set protection enable flag in CR0
    mov eax, cr0
    or al, CR0_PE
    mov cr0, eax

    ; Far jump into protected mode
    jmp dword CODE_SEG_32b:.pmode

.pmode:
    ; We are now in protected mode!
    [bits 32]
    
    ; Setup segment registers
    mov ax, DATA_SEG_32b
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    mov esp, stage2_entry
   
    ; Clear BSS (uninitialized data)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb

    ; Pass boot drive as argument to start function
    xor edx, edx
    mov dl, [g_boot_drive]
    push edx
    
    ; Call C function
    call start

    ; If start returns, halt
.hang:
    cli
    hlt
    jmp .hang


;
; A20 Gate Enable (16-bit real mode)
;
; --- Ports ---
A20_FAST_PORT        equ 0x92     ; Fast A20 gate control port
KBC_CMD_PORT         equ 0x64     ; Keyboard controller command/status port
KBC_DATA_PORT        equ 0x60     ; Keyboard controller data port

; --- Commands ---
KBC_CMD_DISABLE   equ 0xAD     ; Disable keyboard
KBC_CMD_ENABLE    equ 0xAE     ; Enable keyboard
KBC_CMD_READ_OUT  equ 0xD0     ; Read control output port
KBC_CMD_WRITE_OUT equ 0xD1     ; Write control output port

; --- Status bits ---
KBC_STATUS_INPUT_FULL  equ 2   ; Bit 1 = input buffer full
KBC_STATUS_OUTPUT_FULL equ 1   ; Bit 0 = output buffer full

a20_enable:
    [bits 16]
    ; Try fast A20 method first
    in al, A20_FAST_PORT
    or al, 2
    out A20_FAST_PORT, al
    
    ; Fallback: keyboard controller method
    call .a20_wait_input
    mov al, KBC_CMD_DISABLE
    out KBC_CMD_PORT, al

    call .a20_wait_input
    mov al, KBC_CMD_READ_OUT
    out KBC_CMD_PORT, al

    call .a20_wait_output
    in al, KBC_DATA_PORT
    push eax

    call .a20_wait_input
    mov al, KBC_CMD_WRITE_OUT
    out KBC_CMD_PORT, al
    
    call .a20_wait_input
    pop eax
    or al, 2                    ; bit 2 = A20 bit
    out KBC_DATA_PORT, al

    call .a20_wait_input
    mov al, KBC_CMD_ENABLE
    out KBC_CMD_PORT, al

    call .a20_wait_input
    ret


.a20_wait_input:
    [bits 16]
    in al, KBC_CMD_PORT
    test al, KBC_STATUS_INPUT_FULL
    jnz .a20_wait_input
    ret

.a20_wait_output:
    [bits 16]
    in al, KBC_CMD_PORT
    test al, KBC_STATUS_OUTPUT_FULL
    jz .a20_wait_output
    ret


;
; GDT load 
;
gdt_load:
    [bits 16]
    lgdt [g_gdt_desc]
    ret

;
; GDT descriptor 
;
align 8
g_gdt:
    ; NULL descriptor
    dq 0

    ; 32-bit code segment (0x08)
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0                        ; Base (bits 0-15)
    db 0                        ; Base (bits 16-23)
    db 10011010b                ; Access: present, ring 0, code, executable, readable
    db 11001111b                ; Granularity: 4k pages, 32-bit + Limit (bits 16-19)
    db 0                        ; Base (bits 24-31)

    ; 32-bit data segment (0x10)
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0                        ; Base (bits 0-15)
    db 0                        ; Base (bits 16-23)
    db 10010010b                ; Access: present, ring 0, data, writable
    db 11001111b                ; Granularity: 4k pages, 32-bit + Limit (bits 16-19)
    db 0                        ; Base (bits 24-31)

    ; 16-bit code segment
    dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
    dw 0                        ; base (bits 0-15) = 0x0
    db 0                        ; base (bits 16-23)
    db 10011010b                ; access (present, ring 0, code segment, executable, direction 0, readable)
    db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
    db 0                        ; base high

    ; 16-bit data segment
    dw 0FFFFh                   ; limit (bits 0-15) = 0xFFFFF
    dw 0                        ; base (bits 0-15) = 0x0
    db 0                        ; base (bits 16-23)
    db 10010010b                ; access (present, ring 0, data segment, executable, direction 0, writable)
    db 00001111b                ; granularity (1b pages, 16-bit pmode) + limit (bits 16-19)
    db 0                        ; base high

g_gdt_desc:
    dw g_gdt_desc - g_gdt - 1    ; Limit (size of GDT - 1)
    dd g_gdt                     ; Base address of GDT

g_boot_drive: db 0


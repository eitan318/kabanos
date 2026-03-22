%ifndef MODE_SWITH
%define MODE_SWITH

; mode_switch.inc - Macros for switching between real and protected mode

%macro x86_enter_real_mode 0
    [bits 32]
    
    ; Jump to 16-bit protected mode segment
    jmp word 0x18:.pmode16
    
.pmode16:
    [bits 16]
    
    ; Setup 16-bit data segments
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Disable protected mode bit in cr0
    mov eax, cr0
    and al, ~1
    mov cr0, eax
    
    ; Far jump to real mode (clears instruction prefetch)
    jmp word 0x0000:.rmode
    
.rmode:
    [bits 16]
    
    ; Setup real mode segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Enable interrupts
    sti
%endmacro

%macro x86_enter_protected_mode 0
    [bits 16]
    
    ; Disable interrupts
    cli
    
    ; Set protection enable flag in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax
    
    ; Far jump into 32-bit protected mode
    jmp dword 0x08:.pmode
    
.pmode:
    [bits 32]
    
    ; Setup 32-bit segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
%endmacro


; Convert linear address to segment:offset address
; Args:
;    1 - linear address
;    2 - (out) target segment (e.g. es)
;    3 - target 32-bit register to use (e.g. eax)
;    4 - target lower 16-bit half of #3 (e.g. ax)

%macro linear_to_segment_offset 4

    mov %3, %1      ; linear address to eax
    shr %3, 4
    mov %2, %4
    mov %3, %1      ; linear address to eax
    and %3, 0xf

%endmacro


%endif

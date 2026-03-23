bits 32 

%include "mode_switch.inc"

global bios_vbe_set_mode
global bios_vbe_info_block_get


VBE_GET_CONTROLLER_INFO  equ 0x4F00     ; includes list of all modes
VBE_GET_MODE_INFO        equ 0x4F01    
VBE_SET_MODE             equ 0x4F02

VBE_LINEAR_FRAMEBUFFER   equ 0x4000






; uint32_t x86_set_video_mode(uint16_t mode)
bios_vbe_set_mode:
    push ebp
    mov ebp, esp

    x86_enter_real_mode
    bits 16

    mov ax, 0x4F02              ; VBE Set Mode function
    mov bx, [bp + 8]            ; Mode ID from stack
    or bx, 0x4000               ; VBE_LINEAR_FRAMEBUFFER
    int 0x10

    cmp ax, 0x004F
    je .success
    mov eax, -1
    jmp .exit

.success:
    xor eax, eax

.exit:
    x86_enter_protected_mode
    bits 32

    pop ebp
    ret

; uint32_t x86_get_video_mode_list(void* buffer)
bios_vbe_info_block_get:
    push ebp
    mov ebp, esp

    x86_enter_real_mode
    bits 16

    linear_to_segment_offset [bp + 8], es, eax, di
    
    mov dword [es:di], 'VBE2'   ; Request VBE 2.0+
    mov ax, 0x4F00              ; VBE Get Controller Info
    int 0x10

    cmp ax, 0x004F
    je .success
    mov eax, -1
    jmp .exit

.success:
    xor eax, eax

.exit:
    x86_enter_protected_mode
    bits 32

    pop ebp
    ret

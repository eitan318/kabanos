; i686_io.asm
; NASM syntax

global i686_inb
global i686_inw
global i686_inl
global i686_outb
global i686_outw
global i686_outl
global i686_panic

section .text

i686_inb:
    mov dx, [esp+4]
    in al, dx
    ret

i686_inw:
    mov dx, [esp+4]
    in ax, dx
    ret


i686_inl:
    mov dx, [esp+4]
    in eax, dx
    ret

i686_outb:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx, al
    ret

i686_outw:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, ax
    ret

i686_outl:
    mov dx, [esp+4]
    mov eax, [esp+8]
    out dx, eax
    ret

i686_panic:
    cli
    hlt
    ret

global i686_inb
global i686_inw
global i686_inl
global i686_outb
global i686_outw
global i686_outl
global i686_panic

section .text

; uint8_t i686_inb(uint16_t port)
i686_inb:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port argument
    in al, dx
    pop ebp
    ret

; uint16_t i686_inw(uint16_t port)
i686_inw:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port argument
    in ax, dx
    pop ebp
    ret

; uint32_t i686_inl(uint16_t port)
i686_inl:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port argument
    in eax, dx
    pop ebp
    ret

; void i686_outb(uint16_t port, uint8_t value)
i686_outb:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port
    mov al, [ebp+12]   ; value
    out dx, al
    pop ebp
    ret

; void i686_outw(uint16_t port, uint16_t value)
i686_outw:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port
    mov ax, [ebp+12]   ; value
    out dx, ax
    pop ebp
    ret

; void i686_outl(uint16_t port, uint32_t value)
i686_outl:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8]    ; port
    mov eax, [ebp+12]  ; value
    out dx, eax
    pop ebp
    ret

; void i686_panic(void)
i686_panic:
    cli
    hlt
    ret


bits 32

; Multiboot header constants
ALIG_FLAG   equ 1 << 0
MEMINFO_FLAG equ 1 << 1
FLAGS        equ ALIG_FLAG | MEMINFO_FLAG
MAGIC        equ 0x1BADB002
CHECKSUM     equ -(MAGIC + FLAGS)

; Declare a multiboot header that marks the program as a kernel.
section .multiboot.data
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

extern kmain
extern _kernel_start
extern _kernel_end

; Allocate the initial stack
section .bootstrap_stack nobits
align 16
stack_bottom:
    resb 16384        ; 16 KiB
stack_top:

; Preallocate pages used for paging
section .bss
align 4096
boot_page_directory:
    resb 4096
boot_page_table1:
    resb 4096

; The kernel entry point
section .multiboot.text
global _start
_start:
    ; Physical address of boot_page_table1
    mov edi, (boot_page_table1 - 0xC0000000)
    
    ; First address to map is address 0
    mov esi, 0
    
    ; Map 1023 pages. The 1024th will be the VGA text buffer
    mov ecx, 1023

.loop:
    ; Only map the kernel and low memory
    cmp esi, 0 
    jl .skip
    cmp esi, (_kernel_end - 0xC0000000)
    jge .done_loop
    
    ; Map physical address as "present, writable"
    mov edx, esi
    or edx, 0x003
    mov [edi], edx

.skip:
    ; Size of page is 4096 bytes
    add esi, 4096
    ; Size of entries in boot_page_table1 is 4 bytes
    add edi, 4
    ; Loop to the next entry if we haven't finished
    loop .loop

.done_loop:
    ; ALSO map the boot params page (0x8000 -> 0xC0008000)
    ;    mov dword [boot_page_table1 - 0xC0000000 + 8 * 4], (0x00008000 | 0x003)

    ; Map VGA video memory to 0xC03FF000 as "present, writable"
    ; why not map to 0xc00B8000
    mov dword [boot_page_table1 - 0xC0000000 + 1023 * 4], (0x000B8000 | 0x003)
    
    ; Map the page table to both virtual addresses 0x00000000 and 0xC0000000
    mov dword [boot_page_directory - 0xC0000000 + 0], (boot_page_table1 - 0xC0000000 + 0x003)
    mov dword [boot_page_directory - 0xC0000000 + 768 * 4], (boot_page_table1 - 0xC0000000 + 0x003)
    
    ; Set cr3 to the address of the boot_page_directory
    mov ecx, (boot_page_directory - 0xC0000000)
    mov cr3, ecx
    
    ; Enable paging and the write-protect bit
    mov ecx, cr0
    or ecx, 0x80010000
    mov cr0, ecx
    
    ; Jump to higher half with an absolute jump
    lea ecx, [higher_half]
    jmp ecx

section .text
higher_half:
    ; At this point, paging is fully set up and enabled
    
    ; Unmap the identity mapping as it is now unnecessary
    ; mov dword [boot_page_directory + 0], 0
    
    
    ; Reload cr3 to force a TLB flush so the changes take effect
    mov ecx, cr3
    mov cr3, ecx
    
    ; Set up the stack
    mov esp, stack_top
    
    ; Enter the high-level kernel
    push ebx ; multiboot info
    call kmain
    
    ; Infinite loop if the system has nothing more to do
    cli
.halt:
    hlt
    jmp .halt

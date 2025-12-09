bits 32

; CPU context structure (i686 specific)
; typedef struct {
;     uint32_t eax;
;     uint32_t ebx;
;     uint32_t ecx;
;     uint32_t edx;
;     uint32_t esi;
;     uint32_t edi;
;     uint32_t ebp;
;     uint32_t esp;
;     uint32_t eip;
;     uint32_t eflags;
;     uint32_t cr3;
; } CpuContext;

section .data
align 4
temp_eip: dd 0
temp_eflags: dd 0
temp_esp: dd 0
temp_cr3: dd 0

section .text
global context_switch

; extern void context_switch(CpuContext *current_cpu_context,
;                            CpuContext *next_cpu_context);

context_switch:
    ; === SAVE CURRENT CONTEXT ===
    mov eax, [esp + 4]
    mov [eax + 0], eax
    mov [eax + 4], ebx
    mov [eax + 8], ecx
    mov [eax + 12], edx
    mov [eax + 16], esi
    mov [eax + 20], edi
    mov [eax + 24], ebp
    mov [eax + 28], esp
    mov ecx, [esp]
    mov [eax + 32], ecx
    pushfd
    pop ecx
    mov [eax + 36], ecx
    mov ecx, cr3
    mov [eax + 40], ecx
    
    ; === LOAD NEXT CONTEXT ===
    cli
    mov eax, [esp + 8]          ; eax = next context pointer
    
    ; Read control values into temporary variables
    ; (These are in kernel .data section, mapped in all address spaces)
    mov ecx, [eax + 28]
    mov [temp_esp], ecx
    mov ecx, [eax + 32]
    mov [temp_eip], ecx
    mov ecx, [eax + 36]
    mov [temp_eflags], ecx
    mov ecx, [eax + 40]
    mov [temp_cr3], ecx
    
    ; Load general registers
    mov ebx, [eax + 4]
    mov ecx, [eax + 8]
    mov edx, [eax + 12]
    mov esi, [eax + 16]
    mov edi, [eax + 20]
    mov ebp, [eax + 24]
    mov eax, [eax + 0]
    
    ; Switch page tables
    push eax
    mov eax, [temp_cr3]
    mov cr3, eax
    pop eax
    
    ; Switch stack
    mov esp, [temp_esp]
    
    ; Push eip and eflags onto new stack
    push dword [temp_eflags]
    push dword [temp_eip]
    
    ; Restore and jump
    pop dword [temp_eip]        ; pop eip into temp
    popfd
    sti
    jmp [temp_eip]

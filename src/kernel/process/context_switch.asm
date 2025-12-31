; bits 32
;
; ; CPU context structure (i686 specific)
; ; typedef struct {
; ;     uint32_t eax;
; ;     uint32_t ebx;
; ;     uint32_t ecx;
; ;     uint32_t edx;
; ;     uint32_t esi;
; ;     uint32_t edi;
; ;     uint32_t ebp;
; ;     uint32_t esp;
; ;     uint32_t eip;
; ;     uint32_t eflags;
; ;     uint32_t cr3;
; ; } CpuContext;
;
; global context_switch
;
; ; extern void context_switch(CpuContext *current_cpu_context,
; ;                            CpuContext *next_cpu_context);
;
; context_switch:
;     ; === SAVE CURRENT CONTEXT ===
;     mov eax, [esp + 4]
;     mov [eax + 0], eax
;     mov [eax + 4], ebx
;     mov [eax + 8], ecx
;     mov [eax + 12], edx
;     mov [eax + 16], esi
;     mov [eax + 20], edi
;     mov [eax + 24], ebp
;     mov [eax + 28], esp
;     mov ecx, [esp]
;     mov [eax + 32], ecx
;     pushfd
;     pop ecx
;     mov [eax + 36], ecx
;     mov ecx, cr3
;     mov [eax + 40], ecx
;
;     ; === LOAD NEXT CONTEXT ===
;     cli
;     mov esi, [esp + 8]          ; esi = next context pointer
;
;     ; Step 1: Load general registers (not esp, not eax yet)
;     mov ebx, [esi + 4]
;     mov ecx, [esi + 8]
;     mov edx, [esi + 12]
;     mov edi, [esi + 20]
;     mov ebp, [esi + 24]
;
;     ; Step 2: Switch page tables
;     mov eax, [esi + 40]
;     mov cr3, eax
;
;     ; Step 3: Switch stack
;     mov esp, [esi + 28]
;
;     ; Step 4: Push eflags and eip onto NEW stack (read from esi before loading esi)
;     push dword [esi + 36]       ; new eflags
;     push dword [esi + 32]       ; new eip
;
;     ; Step 5: Load remaining registers
;     mov eax, [esi + 0]
;     mov esi, [esi + 16]
;
;     ; Step 6: Restore eflags and jump to new eip
;     pop ecx                     ; ecx = new eip
;     popfd                       ; restore eflags
;     sti                         ; re-enable interrupts
;     jmp ecx                     ; jump to new eip

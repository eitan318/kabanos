bits 32

;
; This should match the isr.asm common_isr for preeamptive switching
;
global switch_to
switch_to:
    ; Input: [esp + 4] = thread_t *next
    mov eax, [esp + 4]          ; EAX = next thread
    
    ; Get process from thread
    mov ebx, [eax + 4]          ; EBX = next->process
    
    ; Load CR3 from process->page_dir
    mov ecx, [ebx + 4]          ; ECX = process->(vmspace*)
    mov ecx, [ecx + 4]          ; ECX = process->vmspace->pd_phys 
    mov cr3, ecx                ; Switch address space
    
    ; Load kernel ESP (points to saved interrupt frame)
    mov esp, [eax + 8]          ; ESP = next->kernel_esp
    
    ; Restore CPU state from interrupt frame
    pop ds                      
    popa                        
    add esp, 8                  ; Skip error code + interrupt number
    iret                        


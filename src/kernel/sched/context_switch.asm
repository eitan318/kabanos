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
    pop ds                      ; Restore DS
    popa                        ; Restore general registers
    add esp, 8                  ; Skip error code + interrupt number
    iret                        ; Return to user/kernel mode
; */
;
;
;
; typedef struct thread {
;
;   uint32_t tid;
;
;   process_t *process;
;
;   // CPU state
;
;   struct cpu_context {
;
;     uint32_t eip;
;
;     uint32_t esp;
;
;     uint32_t ebp;
;
;     uint32_t eflags;
;
;     uint32_t regs[8];
;
;   } context;
;
;   enum thread_state { THREAD_READY, THREAD_RUNNING } state;
;
;   enum thread_mode { TASK_MODE_KERNEL, TASK_MODE_USER } mode;
;
;   // kernel stack pointer
;
;   void *kstack;
;
;   struct thread *next; // runqueue
;
; } thread_t;
;
;


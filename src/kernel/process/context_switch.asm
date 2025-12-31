bits 32

global switch_to
switch_to:
      pusha                 ; save registers to curr stack

      mov eax, [esp + 32 + 4]   ; eax = current
      mov edx, [esp + 32 + 8]   ; edx = next
      mov [eax], esp        ; save ESP in current->esp

      mov esp, [edx]        ; load ESP of next
      popa                  ; restore registers of next stack
      ret


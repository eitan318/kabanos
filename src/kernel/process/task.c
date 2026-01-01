#include "task.h"
#include "arch/i686/isr/isr.h"

#define STACK_SIZE 10000

// this shouild match isr.asm isr_common for preeamptive schedualing
void setup_task(Task *t, void (*entry)(void), uint8_t *stack) {
  uint32_t *stk = (uint32_t *)(stack + STACK_SIZE);

  // ---- iret frame ----
  *(--stk) = 0x202;           // EFLAGS
  *(--stk) = KERNEL_CS;       // CS
  *(--stk) = (uint32_t)entry; // EIP

  // ---- interrupt frame ----
  *(--stk) = 0;              // error
  *(--stk) = PREEMPTIVE_INT; // int number

  // ---- saved DS ----
  *(--stk) = KERNEL_DS;

  // ---- pusha frame ----
  *(--stk) = 0; // edi
  *(--stk) = 0; // esi
  *(--stk) = 0; // ebp
  *(--stk) = 0; // esp dummy
  *(--stk) = 0; // ebx
  *(--stk) = 0; // edx
  *(--stk) = 0; // ecx
  *(--stk) = 0; // eax

  t->kernel_esp = stk;
}

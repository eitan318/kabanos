#include "task.h"

#define STACK_SIZE 10000

void setup_task(Task *t, void (*entry)(void), uint8_t *stack) {
  uint32_t *stk = (uint32_t *)(stack + STACK_SIZE);

  *(--stk) = (uint32_t)entry; // return address for ret

  // Push fake registers for initial pusha/popa
  *(--stk) = 0; // edi
  *(--stk) = 0; // esi
  *(--stk) = 0; // ebp
  *(--stk) = 0; // esp placeholder (not used)
  *(--stk) = 0; // ebx
  *(--stk) = 0; // edx
  *(--stk) = 0; // ecx
  *(--stk) = 0; // eax

  t->esp = (uint32_t)stk;
}

#include "task.h"

#define STACK_SIZE 10000

void setup_task(Task *t, void (*entry)(void), uint8_t *stack) {
  uint32_t *stk = (uint32_t *)(stack + STACK_SIZE);
  *(--stk) = (uint32_t)entry; // return address
  t->esp = (uint32_t)stk;
}

__attribute__((naked)) void switch_to(Task *current, Task *next) {
  asm volatile("mov 4(%%esp), %%eax\n" // current
               "mov %%esp, (%%eax)\n"  // save ESP

               "mov 8(%%esp), %%eax\n" // next
               "mov (%%eax), %%esp\n"  // load ESP

               "ret\n"
               :
               :
               : "memory", "eax");
}

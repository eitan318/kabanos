#ifndef _SETJMP_H
#define _SETJMP_H

// A buffer to hold registers: EBP, EBX, EDI, ESI, ESP, EIP
typedef int jmp_buf[6];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif

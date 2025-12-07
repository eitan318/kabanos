// calc.c
void _start(void)
{
    int a = 1 + 2;
    int b = 5 - 3;
    int c = a * b;
    // int z = c / 0;
    
    // Halt when done (prevent returning to nowhere)
    while(1) {
        __asm__ volatile("hlt");
    }
    
    // If you want to "return" to kernel, you'd need a syscall mechanism
    // For now, we just halt
}

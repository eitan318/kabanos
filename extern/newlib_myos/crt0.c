__asm__(".section .text\n"
        ".global _start\n"
        "_start:\n"
        "    call main\n"
        "    \n"
        "    push %eax\n"
        "    call exit\n");

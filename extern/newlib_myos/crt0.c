/* crt0.c - Kabanos OS entry point */

__asm__(".section .text\n"
        ".global _start\n"
        "_start:\n"
        "    mov (%esp), %edi\n"           /* argc */
        "    lea 4(%esp), %esi\n"          /* argv */
        "    lea 8(%esp, %edi, 4), %edx\n" /* envp */
        "    \n"
        "    /* Align stack to 16 bytes for ABI compliance */\n"
        "    and $-16, %esp\n"
        "    \n"
        "    push %edx\n"
        "    push %esi\n"
        "    push %edi\n"
        "    \n"
        "    call main\n"
        "    \n"
        "    push %eax\n"
        "    call _exit\n");

section .text
global _start
extern main
extern _exit

_start:
    mov rdi, [rsp]          ; First argument: argc is at the top of the stack
    lea rsi, [rsp + 8]      ; Second argument: argv starts right after argc
    lea rdx, [rsp + 16 + rdi*8] ; Third argument: envp starts after argv's NULL terminator
    
    call main
    
    mov rdi, rax            ; Move return value of main to rdi (exit code)
    call _exit

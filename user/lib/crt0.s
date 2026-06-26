.global _start
_start:
    call main

    mov %rax, %rdi
    mov $0, %rax
    
    syscall
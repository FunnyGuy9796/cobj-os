.extern syscall_dispatch
.extern cpu_local

.global syscall_entry
syscall_entry:
    swapgs
    mov %rsp, %gs:16
    mov %gs:8,  %rsp
    sti

    push %rcx
    push %r11
    push %gs:16
    push %rax
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    sub $8, %rsp
    
    mov %r9, %rax
    mov %r8, %r9
    mov %r10, %r8
    mov %rdx, %rcx
    mov %rsi, %rdx
    mov %rdi, %rsi
    mov 6*8+8(%rsp), %rdi
    push %rax

    call syscall_dispatch

    add $8, %rsp
    add $8, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    add $8, %rsp
    pop %gs:16
    pop %r11
    pop %rcx

    cli
    mov %gs:16, %rsp
    swapgs
    sysretq

.global jump_userspace
jump_userspace:
    push $0x1b
    push %rsi
    push $0x202
    push $0x23
    push %rdi
    
    iretq
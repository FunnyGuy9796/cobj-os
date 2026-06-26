.global thread_switch
thread_switch:
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15

    mov %rsp, 0(%rdi)

    mov 0(%rsi), %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    
    ret

.global thread_idle
thread_idle:
    mov 0(%rdi), %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    
    ret

.global thread_entry_trampoline
thread_entry_trampoline:
    iretq
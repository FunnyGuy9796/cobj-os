#include "proc.h"

uint64_t spawn(const char *name, const char **argv, int argc) {
    const char *new_argv[argc + 1];
    uint64_t ret;

    new_argv[0] = name;

    for (int i = 0; i < argc; i++)
        new_argv[i + 1] = argv ? argv[i] : NULL;

    __asm__ volatile (
        "mov $4, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(name), "S"(new_argv), "d"((uint64_t)(argc + 1))
        : "rcx", "r11", "memory"
    );

    return ret;
}

uint64_t getpid() {
    uint64_t ret;

    __asm__ volatile (
        "mov $5, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        :
        : "rcx", "r11", "memory"
    );

    return ret;
}

int waitpid(uint64_t pid) {
    int ret;

    __asm__ volatile (
        "mov $6, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(pid)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int kill(uint64_t pid) {
    int ret;

    __asm__ volatile (
        "mov $10, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(pid)
        : "rcx", "r11", "memory"
    );
}

int listprocs(proc_info_t *buf, int max) {
    int ret;

    __asm__ volatile (
        "mov $15, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(buf), "S"((uint64_t)max)
        : "rcx", "r11", "memory"
    );
    
    return ret;
}
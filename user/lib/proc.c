#include "proc.h"

uint64_t spawn(const char *name) {
    uint64_t ret;

    __asm__ volatile (
        "mov $4, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(name)
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
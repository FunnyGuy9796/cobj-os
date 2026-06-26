#include "io.h"

char read_char() {
    char ret;

    __asm__ volatile (
        "mov $7, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        :
        : "rcx", "r11", "memory"
    );

    return ret;
}
#include "sys.h"

uint64_t getuptime() {
    uint64_t ret;

    __asm__ volatile (
        "mov $8, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        :
        : "rcx", "r11", "memory"
    );

    return ret;
}

void gettime(rtc_time_t *out) {
    __asm__ volatile (
        "mov $14, %%rax\n"
        "syscall\n"
        :: "D"(out)
        : "rax", "rcx", "r11", "memory"
    );
}

int sleep(uint64_t ms) {
    int ret;

    __asm__ volatile (
        "mov $9, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(ms)
        : "rcx", "r11", "memory"
    );
}
#include "port.h"

port_token_t create_port() {
    uint64_t ret;

    __asm__ volatile (
        "mov $17, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : 
        : "rcx", "r11", "memory"
    );

    return (port_token_t)ret;
}

int forward_port(port_token_t dst, void *msg, size_t len) {
    uint64_t ret;

    __asm__ volatile (
        "mov $18, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(dst), "S"(msg), "d"(len)
        : "rcx", "r11", "memory"
    );

    return (int)ret;
}

int receive_port(port_token_t token, void *buf, size_t len) {
    uint64_t ret;

    __asm__ volatile (
        "mov $19, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(token), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );

    return (int)ret;
}

int destroy_port(port_token_t token) {
    uint64_t ret;

    __asm__ volatile (
        "mov $20, %%rax\n"
        "mov %1, %%rdi\n"
        "syscall\n"
        : "=a"(ret)
        : "r"(token)
        : "rcx", "r11", "rdi", "memory"
    );

    return (int)ret;
}
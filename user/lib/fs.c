#include "fs.h"

int open(const char *path) {
    int ret;

    __asm__ volatile (
        "mov $11, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(path)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int64_t read(int fd, void *buf, uint64_t size) {
    int64_t ret;

    __asm__ volatile (
        "mov $13, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"((uint64_t)fd), "S"(buf), "d"(size)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int64_t write(int fd, const void *buf, uint64_t size) {
    int64_t ret;

    __asm__ volatile (
        "mov $16, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"((uint64_t)fd), "S"(buf), "d"(size)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int close(int fd) {
    int ret;

    __asm__ volatile (
        "mov $12, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"((uint64_t)fd)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int listdir(const char *path, dirent_t *entries, int max) {
    int ret;

    __asm__ volatile (
        "mov $7, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(path), "S"(entries), "d"((uint64_t)max)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int getcwd(char *path) {
    int ret;

    __asm__ volatile (
        "mov $21, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(path)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int setcwd(const char *path, size_t len) {
    int ret;

    __asm__ volatile (
        "mov $22, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(path), "S"((uint64_t)len)
        : "rcx", "r11", "memory"
    );
}
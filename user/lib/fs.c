#include "fs.h"
#include "util.h"
#include "mem.h"
#include "printf.h"

int join(char *out, size_t out_size, const char *cwd, const char *input) {
    char drive[8];
    const char *rest;

    if (strchr(input, ':')) {
        strncpy(out, input, out_size - 1);

        out[out_size - 1] = '\0';

        return 0;
    }

    const char *colon = strchr(cwd, ':');

    if (!colon)
        return -1;

    size_t drive_len = (size_t)(colon - cwd) + 1;

    if (drive_len >= sizeof(drive))
        return -1;

    memcpy(drive, cwd, drive_len);
    drive[drive_len] = '\0';

    char comps[32][64];
    int ncomps = 0;

    const char *p = colon + 1;
    char tok[64];
    int ti = 0;

    #define PUSH_TOK() do { \
        if (ti > 0) { \
            tok[ti] = '\0'; \
            if (strcmp(tok, ".") == 0) { \
                /* no-op */ \
            } else if (strcmp(tok, "..") == 0) { \
                if (ncomps > 0) ncomps--; \
            } else { \
                if (ncomps >= 32) return -1; \
                strncpy(comps[ncomps], tok, sizeof(comps[ncomps]) - 1); \
                comps[ncomps][sizeof(comps[ncomps]) - 1] = '\0'; \
                ncomps++; \
            } \
            ti = 0; \
        } \
    } while (0)

    if (input[0] != '/') {
        while (*p) {
            if (*p == '/') {
                PUSH_TOK();
            } else if (ti < (int)sizeof(tok) - 1) {
                tok[ti++] = *p;
            }
            p++;
        }
        PUSH_TOK();
    }

    p = input;
    ti = 0;

    while (*p) {
        if (*p == '/') {
            PUSH_TOK();
        } else if (ti < (int)sizeof(tok) - 1) {
            tok[ti++] = *p;
        }
        p++;
    }
    PUSH_TOK();

    #undef PUSH_TOK

    size_t off = 0;
    int n = snprintf(out, out_size, "%s/", drive);

    if (n < 0 || (size_t)n >= out_size)
        return -1;

    off = (size_t)n;

    for (int i = 0; i < ncomps; i++) {
        n = snprintf(out + off, out_size - off, "%s%s", comps[i], (i == ncomps - 1) ? "" : "/");

        if (n < 0 || (size_t)n >= out_size - off)
            return -1;

        off += (size_t)n;
    }

    return 0;
}

int open(const char *path) {
    int ret;
    char resolved[256];

    if (!strchr(path, ':')) {
        char cwd[256];

        if (getcwd(cwd) < 0)
            return -1;

        if (join(resolved, sizeof(resolved), cwd, path) < 0)
            return -1;

        path = resolved;
    }

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
    char resolved[256];

    if (!strchr(path, ':')) {
        char cwd[256];

        if (getcwd(cwd) < 0)
            return -1;

        if (join(resolved, sizeof(resolved), cwd, path) < 0)
            return -1;

        path = resolved;
    }

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
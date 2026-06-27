#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint64_t pid;
    char name[64];
    uint8_t state;
} proc_info_t;

uint64_t spawn(const char *name, const char **argv, int argc);
uint64_t getpid();
int waitpid(uint64_t pid);
int kill(uint64_t pid);
int listprocs(proc_info_t *buf, int max);

#endif
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    uint64_t self;
    uint64_t kernel_rsp;
    uint64_t user_rsp;
} cpu_local_t;

extern cpu_local_t cpu_local;

void cpu_init();

static inline void wrmsr(uint32_t msr, uint64_t val) {
    __asm__ volatile ("wrmsr" :: "c"(msr), "a"((uint32_t)val), "d"((uint32_t)(val >> 32)));
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;

    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));

    return ((uint64_t)hi << 32) | lo;
}

#endif
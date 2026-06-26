#include "cpu.h"
#include "../misc/printf.h"

#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_SFMASK 0xc0000084
#define MSR_GS_BASE 0xc0000101
#define MSR_KERNEL_GS 0xc0000102

cpu_local_t cpu_local = {0};

static void cpu_local_init() {
    cpu_local.self = (uint64_t)&cpu_local;
    cpu_local.kernel_rsp = 0;
    cpu_local.user_rsp = 0;

    wrmsr(MSR_GS_BASE, (uint64_t)&cpu_local);
    wrmsr(MSR_KERNEL_GS, (uint64_t)&cpu_local);
}

extern void syscall_entry();

static void syscall_init() {
    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | 1);
    wrmsr(MSR_STAR, ((uint64_t)0x08 << 32) | ((uint64_t)0x10 << 48));

    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);
    wrmsr(MSR_SFMASK, (1 << 9));
}

void cpu_init() {
    cpu_local_init();
    syscall_init();
}
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "thread.h"
#include "../mm/vmm.h"
#include "../loader/elf.h"
#include "../userspace/cpu.h"
#include "../arch/tss.h"
#include "../fs/fs.h"
#include "../misc/printf.h"

#define MAX_FDS 64

typedef struct vm_region {
    uint64_t base;
    uint64_t size;
    struct vm_region *next;
} vm_region_t;

typedef struct process {
    uint64_t pid;
    addr_space_t *addr_space;
    thread_t *thread;
    uint64_t heap_top;
    vm_region_t *regions;
    file_t *fds[MAX_FDS];
    struct process *next;
} process_t;

uint64_t proc_create(const uint8_t *elf_data, uint64_t elf_size);
process_t *proc_find(uint64_t pid);

#endif
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "thread.h"
#include "../mm/vmm.h"
#include "../loader/elf.h"
#include "../userspace/cpu.h"
#include "../userspace/port.h"
#include "../arch/tss.h"
#include "../fs/vfs.h"
#include "../fs/tar_vfs.h"
#include "../misc/printf.h"

#define MAX_FDS 64

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_RESERVED 2

#define MAX_PORTS 64

typedef struct port port_t;

typedef struct vm_region {
    uint64_t base;
    uint64_t size;
    struct vm_region *next;
} vm_region_t;

typedef struct process {
    uint64_t pid;
    char name[64];
    addr_space_t *addr_space;
    thread_t *thread;
    uint64_t heap_top;
    vm_region_t *regions;
    file_t *fds[MAX_FDS];
    port_t *ports[MAX_PORTS];
    char cwd[256];
    struct process *next;
} process_t;

typedef struct {
    uint64_t pid;
    char name[64];
    uint8_t state;
} proc_info_t;

extern process_t *proc_list;
extern volatile bool proc_needs_cleanup;

uint64_t proc_create(const uint8_t *elf_data, uint64_t elf_size, const char **argv, int argc, const char *parent_cwd);
process_t *proc_find(uint64_t pid);
void proc_cleanup();

#endif
#include "process.h"

process_t *curr_proc = NULL;

static uint64_t next_proc_id = 0;
static process_t *proc_list = NULL;

extern void jump_userspace(uint64_t rip, uint64_t rsp);

uint64_t proc_create(const uint8_t *elf_data, uint64_t elf_size) {
    addr_space_t *space = vmm_create_user_space();
    uint64_t entry = elf_load(space, elf_data, elf_size);

    if (!entry) {
        serial_printf("[E] process.c: proc_create() -> elf load failed\n");

        return -1;
    }

    process_t *proc = kmalloc(sizeof(process_t));

    if (!proc)
        panic("process.c: proc_create() -> failed to allocate process\n");

    uint64_t stack_phys = pmm_alloc_pages(4);

    if (!stack_phys)
        panic("process.c: proc_create() -> failed to allocate user stack\n");

    uint64_t stack_top = 0x7fffff000000ULL;

    for (int i = 0; i < 4; i++)
        vmm_map_page(space, stack_top - (4 - i) * 4096, stack_phys + i * 4096, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    
    thread_t *thread = thread_create_user(entry, stack_top - 16);
    
    proc->pid = next_proc_id++;
    proc->addr_space = space;
    proc->thread = thread;
    proc->heap_top = 0x10000000ULL;

    memset(proc->fds, 0, sizeof(proc->fds));

    proc->next = proc_list;
    proc_list = proc;

    thread->process = proc;

    runqueue_push(thread);

    return proc->pid;
}

process_t *proc_find(uint64_t pid) {
    process_t *p = proc_list;
    
    while (p) {
        if (p->pid == pid)
            return p;

        p = p->next;
    }

    return NULL;
}
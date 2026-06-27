#include "process.h"

process_t *proc_list = NULL;

static process_t *curr_proc = NULL;
static uint64_t next_proc_id = 0;

extern void jump_userspace(uint64_t rip, uint64_t rsp);

uint64_t proc_create(const uint8_t *elf_data, uint64_t elf_size, const char **argv, int argc) {
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
    uint64_t stack_base = stack_top - 4 * PAGE_SIZE;

    for (int i = 0; i < 4; i++)
        vmm_map_page(space, stack_base + i * PAGE_SIZE, stack_phys + i * PAGE_SIZE, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    
    uint8_t *stack_virt = (uint8_t *)(stack_phys + hhdm_offset);
    uint64_t str_off = 4 * PAGE_SIZE;
    uint64_t ptrs[argc + 1];

    if (argc > 0 && argv != NULL) {
        for (int i = argc - 1; i >= 0; i--) {
            int len = strlen(argv[i]) + 1;

            str_off -= len;
            str_off &= ~0x7ULL;

            memcpy(stack_virt + str_off, argv[i], len);

            ptrs[i] = stack_base + str_off;
        }
    }

    ptrs[argc] = 0;

    str_off -= (argc + 1) * 8;
    str_off &= ~0xfULL;

    memcpy(stack_virt + str_off, ptrs, (argc + 1) * 8);

    uint64_t argv_user = stack_base + str_off;
    uint64_t user_rsp = stack_base + str_off - 16;
    
    thread_t *thread = thread_create_user(entry, user_rsp);
    
    thread->user_argc = argc;
    thread->user_argv = argv_user;
    
    strncpy(proc->name, argc > 0 ? argv[0] : "unknown", 63);

    proc->name[63] = '\0';

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

void proc_cleanup() {
    process_t **curr = &proc_list;

    while (*curr) {
        process_t *proc = *curr;

        if (proc->thread->state == THREAD_DEAD) {
            for (int i = 0; i < MAX_FDS; i++) {
                if (proc->fds[i]) {
                    kfree(proc->fds[i]);

                    proc->fds[i] = NULL;
                }
            }

            vm_region_t *region = proc->regions;

            while (region) {
                vm_region_t *next = region->next;

                kfree(region);

                region = next;
            }

            pmm_free_pages((uint64_t)proc->thread->kernel_stack - hhdm_offset, proc->thread->stack_size / PAGE_SIZE);

            kfree(proc->thread);

            *curr = proc->next;

            kfree(proc);
        } else
            curr = &proc->next;
    }
}
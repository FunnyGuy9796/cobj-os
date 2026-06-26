#include "syscall.h"
#include "../misc/printf.h"
#include "../multi/thread.h"
#include "../multi/process.h"
#include "../fs/tar.h"
#include "../mm/heap.h"

void sys_exit(uint64_t code) {
    uint64_t dying_pid = curr_thread->process->pid;

    runqueue_remove(curr_thread);
    
    curr_thread->state = THREAD_DEAD;

    thread_wake_pid(dying_pid);

    __asm__ volatile (
        "mov %0, %%rsp\n"
        "call schedule\n"
        "ud2\n"
        :: "r"(idle_thread->kernel_rsp_top)
        : "memory"
    );

    __builtin_unreachable();
}

void sys_print(const char *str) {
    serial_write(str);
}

void *sys_mmap(uint64_t addr, uint64_t size) {
    process_t *proc = curr_thread->process;
    uint64_t pages = (size + 4095) / PAGE_SIZE;
    uint64_t vaddr = addr ? addr : proc->heap_top;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t phys = pmm_alloc_page();

        vmm_map_page(proc->addr_space, vaddr + i * PAGE_SIZE, phys, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    proc->heap_top = vaddr + pages * PAGE_SIZE;

    vm_region_t *region = kmalloc(sizeof(vm_region_t));

    region->base = vaddr;
    region->size = pages * PAGE_SIZE;
    region->next = proc->regions;
    proc->regions = region;

    return (void *)vaddr;
}

int sys_munmap(uint64_t addr, uint64_t size) {
    process_t *proc = curr_thread->process;
    uint64_t pages = (size + 4095) / PAGE_SIZE;
    vm_region_t **curr = &proc->regions;

    while (*curr) {
        if ((*curr)->base == addr) {
            vm_region_t *found = *curr;

            *curr = found->next;

            kfree(found);

            break;
        }

        curr = &(*curr)->next;
    }

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t vaddr = addr + i * PAGE_SIZE;
        uint64_t *pte = vmm_get_pte(proc->addr_space, vaddr, 0);

        if (!pte || !(*pte & PAGE_PRESENT))
            continue;
        
        uint64_t phys = *pte & ~0xfffULL;

        pmm_free_page(phys);
        vmm_unmap_page(proc->addr_space, vaddr);
    }

    return 0;
}

uint64_t sys_spawn(const char *name) {
    uint64_t size = 0;
    void *elf = tar_find(name, &size);

    if (!elf) {
        serial_printf("[E] syscall.c: sys_spawn() -> '%s' not found\n", name);

        return -1;
    }

    uint64_t pid = proc_create(elf, size);

    return pid;
}

uint64_t sys_getpid() {
    return curr_thread->process->pid;
}

void sys_wait_pid(uint64_t pid) {
    process_t *target = proc_find(pid);

    if (!target)
        return;
    
    curr_thread->state = THREAD_BLOCKED;
    curr_thread->waiting_for = target->pid;

    schedule();
    
    cpu_local.kernel_rsp = curr_thread->kernel_rsp_top;
    
    tss_set_rsp0(curr_thread->kernel_rsp_top);
}

char sys_read_char() {
    while (!(inb(0x3f8 + 5) & 1));

    return inb(0x3f8);
}

uint64_t syscall_dispatch(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    switch (number) {
        case SYS_EXIT: {
            sys_exit(arg0);

            return 0;
        }

        case SYS_PRINT: {
            sys_print((const char *)arg0);

            return 0;
        }

        case SYS_MMAP:
            return (uint64_t)sys_mmap(arg0, arg1);
        
        case SYS_MUNMAP:
            return (uint64_t)sys_munmap(arg0, arg1);

        case SYS_SPAWN:
            return sys_spawn((const char *)arg0);
        
        case SYS_GETPID:
            return sys_getpid();
        
        case SYS_WAIT_PID: {
            sys_wait_pid(arg0);

            return 0;
        }
        
        case SYS_READ_CHAR:
            return (uint64_t)sys_read_char();

        default: {
            serial_printf("[D] syscall number=%d arg0=%p\n", number, arg0);

            break;
        }
    }

    return -1;
}
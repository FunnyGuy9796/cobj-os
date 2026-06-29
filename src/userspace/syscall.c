#include "syscall.h"
#include "port.h"
#include "../misc/printf.h"
#include "../multi/thread.h"
#include "../multi/process.h"
#include "../fs/tar.h"
#include "../mm/heap.h"
#include "../arch/idt.h"
#include "../arch/rtc.h"

void sys_exit(uint64_t code) {
    process_t *proc = curr_thread->process;
    uint64_t dying_pid = proc->pid;

    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i]) {
            port_destroy(proc->ports[i]);

            proc->ports[i] = NULL;
        }
    }

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

uint64_t sys_spawn(const char *name, const char **argv, int argc) {
    uint64_t size = 0;
    void *elf = tar_find(name, &size);

    if (!elf)
        return (uint64_t)-1;

    char **argv_copies = kmalloc(sizeof(char *) * argc);

    for (int i = 0; i < argc; i++) {
        int len = strlen(argv[i]);

        if (len > 255)
            len = 255;

        argv_copies[i] = kmalloc(len + 1);

        memcpy(argv_copies[i], argv[i], len);

        argv_copies[i][len] = '\0';
    }

    uint64_t pid = proc_create(elf, size, (const char **)argv_copies, argc);

    for (int i = 0; i < argc; i++)
        kfree(argv_copies[i]);

    kfree(argv_copies);

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
}

char sys_read_char() {
    while (!(inb(0x3f8 + 5) & 1));

    return inb(0x3f8);
}

void sys_sleep(uint64_t ms) {
    curr_thread->sleep_until = apic_timer_ticks() + ms;
    curr_thread->state = THREAD_BLOCKED;

    schedule();
}

int sys_kill(uint64_t pid) {
    process_t *proc = proc_find(pid);

    if (!proc)
        return -1;

    runqueue_remove(proc->thread);

    proc->thread->state = THREAD_DEAD;

    if (proc->thread == curr_thread) {
        __asm__ volatile (
            "mov %0, %%rsp\n"
            "call schedule\n"
            "ud2\n"
            :: "r"(idle_thread->kernel_rsp_top)
            : "memory"
        );

        __builtin_unreachable();
    }

    return 0;
}

int sys_open(const char *path) {
    process_t *proc = curr_thread->process;
    int fd = -1;

    for (int i = FD_RESERVED; i < MAX_FDS; i++) {
        if (!proc->fds[i]) {
            fd = i;

            break;
        }
    }

    if (fd == -1)
        return -1;

    uint64_t size = 0;
    void *data = tar_find(path, &size);

    if (!data)
        return -1;

    file_t *file = kmalloc(sizeof(file_t));

    file->data = data;
    file->size = size;
    file->offset = 0;

    proc->fds[fd] = file;

    return fd;
}

int64_t sys_read(int fd, void *buf, uint64_t size) {
    if (fd == FD_STDIN) {
        char *cbuf = (char *)buf;
        uint64_t i = 0;

        while (i < size) {
            cbuf[i] = sys_read_char();

            if (cbuf[i++] == '\n')
                break;
        }

        return (int64_t)i;
    }

    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;
    
    file_t *file = proc->fds[fd];
    uint64_t remaining = file->size - file->offset;
    uint64_t to_read = size < remaining ? size : remaining;

    if (to_read == 0)
        return 0;
    
    memcpy(buf, (uint8_t *)file->data + file->offset, to_read);

    file->offset += to_read;

    return (int64_t)to_read;
}

int64_t sys_write(int fd, const void *buf, uint64_t size) {
    if (fd == FD_STDOUT || fd == FD_STDERR) {
        const char *cbuf = (const char *)buf;

        for (uint64_t i = 0; i < size; i++)
            serial_putchar(cbuf[i]);

        return (int64_t)size;
    }

    return -1;
}

int sys_close(int fd) {
    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;
    
    kfree(proc->fds[fd]);

    proc->fds[fd] = NULL;

    return 0;
}

void sys_gettime(rtc_time_t *out) {
    *out = rtc_now();
}

int sys_listprocs(proc_info_t *buf, int max) {
    process_t *proc = proc_list;
    int count = 0;

    while (proc && count < max) {
        buf[count].pid = proc->pid;
        buf[count].state = (uint8_t)proc->thread->state;

        if (proc->thread && proc->name[0])
            strncpy(buf[count].name, proc->name, 63);
        else
            buf[count].name[0] = '\0';
        
        count++;
        proc = proc->next;
    }

    return count;
}

int sys_listdir(const char *path, tar_entry_t *entries, int max) {
    return tar_listdir(path, entries, max);
}

port_token_t sys_port_create() {
    process_t *proc = curr_thread->process;
    port_t *port = port_create();

    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i] == 0) {
            proc->ports[i] = port;

            return port->token;
        }
    }

    return 0;
}

int sys_port_receive(port_token_t token, void *buf, uint64_t len) {
    process_t *proc = curr_thread->process;

    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i] && proc->ports[i]->token == token)
            return port_receive(token, buf, len);
    }

    return -1;
}

int sys_port_destroy(port_token_t token) {
    process_t *proc = curr_thread->process;

    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i] && proc->ports[i]->token == token) {
            port_destroy(proc->ports[i]);

            proc->ports[i] = NULL;

            return 0;
        }
    }

    return -1;
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
            return sys_spawn((const char *)arg0, (const char **)arg1, (int)arg2);
        
        case SYS_GETPID:
            return sys_getpid();
        
        case SYS_WAIT_PID: {
            sys_wait_pid(arg0);

            return 0;
        }
        
        case SYS_UPTIME:
            return apic_timer_ticks();
        
        case SYS_SLEEP: {
            sys_sleep(arg0);

            return 0;
        }

        case SYS_KILL:
            return (uint64_t)sys_kill(arg0);
        
        case SYS_OPEN:
            return (uint64_t)sys_open((const char *)arg0);
        
        case SYS_READ:
            return (uint64_t)sys_read((int)arg0, (void *)arg1, arg2);

        case SYS_CLOSE:
            return (uint64_t)sys_close((int)arg0);
        
        case SYS_GETTIME: {
            sys_gettime((rtc_time_t *)arg0);

            return 0;
        }

        case SYS_LISTPROCS:
            return (uint64_t)sys_listprocs((proc_info_t *)arg0, (int)arg1);
        
        case SYS_LISTDIR:
            return (uint64_t)sys_listdir((const char *)arg0, (tar_entry_t *)arg1, (int)arg2);
        
        case SYS_WRITE:
            return (uint64_t)sys_write((int)arg0, (const void *)arg1, arg2);
        
        case SYS_PORT_CREATE:
            return (uint64_t)sys_port_create();
        
        case SYS_PORT_FORWARD:
            return (uint64_t)port_forward((port_token_t)arg0, (void *)arg1, arg2);
        
        case SYS_PORT_RECEIVE:
            return (uint64_t)sys_port_receive((port_token_t)arg0, (void *)arg1, arg2);
        
        case SYS_PORT_DESTROY:
            return (uint64_t)sys_port_destroy((port_token_t)arg0);

        default: {
            serial_printf("[D] syscall number=%d arg0=%p\n", number, arg0);

            break;
        }
    }

    return -1;
}
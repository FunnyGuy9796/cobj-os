#include "syscall.h"
#include "port.h"
#include "../misc/printf.h"
#include "../multi/thread.h"
#include "../multi/process.h"
#include "../fs/tar.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../arch/idt.h"
#include "../arch/rtc.h"
#include "../devices/kb.h"

volatile bool proc_needs_cleanup = false;

static int copy_to_user(void *user_dst, const void *kernel_src, size_t len) {
    uint8_t *dst = (uint8_t *)user_dst;
    const uint8_t *src = (const uint8_t *)kernel_src;

    while (len > 0) {
        uintptr_t va = (uintptr_t)dst;
        uintptr_t page_base = va & ~0xfffULL;
        uintptr_t page_off = va - page_base;
        size_t chunk = PAGE_SIZE - page_off;

        if (chunk > len)
            chunk = len;

        if (va >= USER_SPACE_TOP)
            return -1;

        uint64_t phys;
        uint64_t flags;

        if (vmm_get_mapping(curr_thread->process->addr_space, page_base, &phys, &flags) < 0)
            return -1;

        if (!(flags & PAGE_PRESENT) || !(flags & PAGE_USER) || !(flags & PAGE_WRITE))
            return -1;

        void *kdst = (void *)(phys + page_off + hhdm_offset);

        memcpy(kdst, src, chunk);

        dst += chunk;
        src += chunk;
        len -= chunk;
    }

    return 0;
}

int copy_from_user(void *kernel_dst, const void *user_src, size_t len) {
    uint8_t *dst = (uint8_t *)kernel_dst;
    const uint8_t *src = (const uint8_t *)user_src;

    while (len > 0) {
        uintptr_t va = (uintptr_t)src;
        uintptr_t page_base = va & ~0xfffULL;
        uintptr_t page_off = va - page_base;
        size_t chunk = PAGE_SIZE - page_off;

        if (chunk > len)
            chunk = len;

        if (va >= USER_SPACE_TOP)
            return -1;

        uint64_t phys;
        uint64_t flags;

        if (vmm_get_mapping(curr_thread->process->addr_space, page_base, &phys, &flags) < 0)
            return -1;

        if (!(flags & PAGE_PRESENT) || !(flags & PAGE_USER))
            return -1;

        void *ksrc = (void *)(phys + page_off + hhdm_offset);

        memcpy(dst, ksrc, chunk);

        dst += chunk;
        src += chunk;
        len -= chunk;
    }

    return 0;
}

void sys_exit(uint64_t code) {
    process_t *proc = curr_thread->process;
    uint64_t dying_pid = proc->pid;

    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i]) {
            port_destroy(proc->ports[i]);

            proc->ports[i] = NULL;
        }
    }

    for (int i = FD_RESERVED; i < MAX_FDS; i++) {
        if (proc->fds[i]) {
            file_t *f = proc->fds[i];

            f->node->refcount--;

            if (f->node->refcount == 0 && f->node->ops->release)
                f->node->ops->release(f->node);
                
            kfree(f);

            proc->fds[i] = NULL;
        }
    }

    runqueue_remove(curr_thread);
    
    curr_thread->state = THREAD_DEAD;
    proc_needs_cleanup = true;

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
    fbcon_write(str);
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
    fsnode_t *prog_node = vfs_resolve(name);

    if (!prog_node)
        return (uint64_t)-1;
    
    uint64_t prog_size = prog_node->size;
    void *prog_elf = kmalloc(prog_size);
    int n = prog_node->ops->read(prog_node, prog_elf, prog_size, 0);

    if (n < 0 || (uint64_t)n != prog_size)
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
    
    if (prog_node->ops->release)
        prog_node->ops->release(prog_node);

    uint64_t pid = proc_create(prog_elf, prog_size, (const char **)argv_copies, argc, curr_thread->process->cwd);

    for (int i = 0; i < argc; i++)
        kfree(argv_copies[i]);

    kfree(prog_elf);

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

void sys_sleep(uint64_t ms) {
    curr_thread->sleep_until = apic_timer_ticks() + ms;
    curr_thread->state = THREAD_BLOCKED;

    schedule();
}

int sys_kill(uint64_t pid) {
    process_t *proc = proc_find(pid);

    if (!proc)
        return -1;
    
    for (int i = 0; i < MAX_PORTS; i++) {
        if (proc->ports[i]) {
            port_destroy(proc->ports[i]);

            proc->ports[i] = NULL;
        }
    }

    for (int i = FD_RESERVED; i < MAX_FDS; i++) {
        if (proc->fds[i]) {
            file_t *f = proc->fds[i];

            f->node->refcount--;

            if (f->node->refcount == 0 && f->node->ops->release)
                f->node->ops->release(f->node);
                
            kfree(f);

            proc->fds[i] = NULL;
        }
    }

    runqueue_remove(proc->thread);

    proc->thread->state = THREAD_DEAD;
    proc_needs_cleanup = true;

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
    
    fsnode_t *node = vfs_resolve(path);

    if (!node)
        return -1;
    
    file_t *file = kmalloc(sizeof(file_t));

    file->node = node;
    file->offset = 0;
    file->flags = VFS_READ;

    proc->fds[fd] = file;

    return fd;
}

int64_t sys_read(int fd, void *buf, uint64_t size) {
    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;
    
    file_t *file = proc->fds[fd];
    int n = file->node->ops->read(file->node, buf, size, file->offset);

    if (n > 0)
        file->offset += n;
    
    return n;
}

int64_t sys_write(int fd, const void *buf, uint64_t size) {
    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;
    
    file_t *file = proc->fds[fd];
    int n = file->node->ops->write(file->node, buf, size, file->offset);

    if (n > 0)
        file->offset += n;

    return n;
}

int sys_close(int fd) {
    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;
    
    file_t *file = proc->fds[fd];

    file->node->refcount--;

    if (file->node->refcount == 0 && file->node->ops->release)
        file->node->ops->release(file->node);
    
    kfree(file);

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

int sys_listdir(const char *path, dirent_t *entries, int max) {
    fsnode_t *dir = vfs_resolve(path);

    if (!dir)
        return -1;

    if (dir->type != NODE_DIR) {
        if (dir->ops && dir->ops->release)
            dir->ops->release(dir);

        return -1;
    }

    int count = 0;

    while (count < max && dir->ops->readdir(dir, count, &entries[count]) == 0)
        count++;

    if (dir->ops && dir->ops->release)
        dir->ops->release(dir);

    return count;
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

int sys_get_cwd(char *buf, size_t buf_size) {
    process_t *proc = curr_thread->process;
    size_t len = strlen(proc->cwd);

    if (buf_size == 0)
        return -1;
    
    size_t to_copy = (len < buf_size - 1) ? len : buf_size - 1;

    if (copy_to_user(buf, proc->cwd, to_copy) < 0)
        return -1;
    
    char nul = '\0';

    if (copy_to_user(buf + to_copy, &nul, 1) < 0)
        return -1;
    
    return (int)to_copy;
}

int sys_set_cwd(char *buf, size_t buf_size) {
    process_t *proc = curr_thread->process;

    if (buf_size == 0)
        return -1;

    char tmp[256];
    size_t to_copy = (buf_size < sizeof(tmp) - 1) ? buf_size : sizeof(tmp) - 1;

    if (copy_from_user(tmp, buf, to_copy) < 0)
        return -1;

    tmp[to_copy] = '\0';

    fsnode_t *node = vfs_resolve(tmp);

    if (!node)
        return -1;
    
    if (node->type != NODE_DIR) {
        if (node->ops && node->ops->release)
            node->ops->release(node);
        
        return -1;
    }

    if (node->ops && node->ops->release)
        node->ops->release(node);
    
    memcpy(proc->cwd, tmp, to_copy + 1);

    return (int)to_copy;
}

int sys_listdrives(drive_info_t *buf, int max) {
    int count = 0;

    for (int i = 0; i < MAX_DRIVES && count < max; i++) {
        if (!vfs_drives[i].present)
            continue;
        
        buf[count].id = (uint8_t)i;
        buf[count].present = 1;

        strncpy(buf[count].label, vfs_drives[i].label, sizeof(buf[count].label) - 1);

        buf[count].label[sizeof(buf[count].label) - 1] = '\0';
        count++;
    }

    return count;
}

int64_t sys_seek(int fd, int64_t offset, int whence) {
    process_t *proc = curr_thread->process;

    if (fd < 0 || fd >= MAX_FDS || !proc->fds[fd])
        return -1;

    file_t *file = proc->fds[fd];
    int64_t new_off;

    switch (whence) {
        case 0: {
            new_off = offset;
            
            break;
        }

        case 1: {
            new_off = (int64_t)file->offset + offset;
            
            break;
        }

        case 2: {
            new_off = (int64_t)file->node->size + offset;
            
            break;
        }

        default:
            return -1;
    }

    if (new_off < 0)
        return -1;

    file->offset = (size_t)new_off;

    return new_off;
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
            return (uint64_t)sys_listdir((const char *)arg0, (dirent_t *)arg1, (int)arg2);
        
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
        
        case SYS_GET_CWD:
            return (uint64_t)sys_get_cwd((char *)arg0, (size_t)arg1);
        
        case SYS_SET_CWD:
            return (uint64_t)sys_set_cwd((char *)arg0, (size_t)arg1);
        
        case SYS_CLEAR_SCREEN: {
            fbcon_clear();

            return 0;
        }

        case SYS_LISTDRIVES:
            return (uint64_t)sys_listdrives((drive_info_t *)arg0, (int)arg1);
        
        case SYS_SEEK:
            return (uint64_t)sys_seek((int)arg0, (int64_t)arg1, (int)arg2);

        default: {
            fbcon_printf("[D] syscall number=%d arg0=%p\n", number, arg0);

            break;
        }
    }

    return -1;
}
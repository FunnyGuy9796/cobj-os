#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "misc/util.h"
#include "misc/mem.h"
#include "misc/printf.h"
#include "arch/gdt.h"
#include "arch/tss.h"
#include "arch/idt.h"
#include "arch/rtc.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "userspace/cpu.h"
#include "multi/thread.h"
#include "multi/process.h"
#include "fs/tar.h"
#include "fs/vfs.h"
#include "fs/tar_vfs.h"

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_base_revision")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_mmap_request")))
static volatile struct limine_memmap_request mmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_hhdm_request")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_executable_address_request")))
static volatile struct limine_executable_address_request exec_addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_module_request")))
static volatile struct limine_module_request mod_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

struct limine_file *initrd;

void kidle() {
    threads_enabled = true;
    
    tar_init(initrd->address, initrd->size);

    vfs_drives[0].root = tar_get_root();
    vfs_drives[0].present = 1;

    strncpy(vfs_drives[0].label, "initrd", sizeof(vfs_drives[0].label));

    fsnode_t *init_node = vfs_resolve("0:/init/init");

    if (!init_node)
        panic("main.c: kidle() -> init not found\n");
    
    uint64_t init_size = init_node->size;
    void *init_elf = kmalloc(init_size);
    int n = init_node->ops->read(init_node, init_elf, init_size, 0);

    if (n < 0 || (uint64_t)n != init_size)
        panic("main.c: kidle() -> unable to read init binary\n");
    
    if (init_node->ops->release)
        init_node->ops->release(init_node);
    
    const char *argv[] = { "0:/init/init" };

    proc_create(init_elf, init_size, argv, 1, NULL);
    kfree(init_elf);

    for (;;)
        __asm__ volatile ("hlt");
}

void kmain() {
    serial_init();

    if (mmap_request.response == NULL || mmap_request.response->entry_count < 1)
        panic("main.c: kmain() -> no memory map\n");

    if (hhdm_request.response == NULL)
        panic("main.c: kmain() -> no hhdm\n");

    if (exec_addr_request.response == NULL)
        panic("main.c: kmain() -> no executable address\n");
    
    if (mod_request.response == NULL || mod_request.response->module_count < 1)
        panic("main.c: kmain() -> no initrd loaded\n");
    
    hhdm_offset = hhdm_request.response->offset;
    initrd = mod_request.response->modules[0];

    gdt_init();
    idt_init();
    tss_init();
    gdt_load_tss();
    pmm_init(mmap_request.response, exec_addr_request.response->physical_base, (void *)((uintptr_t)initrd->address - hhdm_offset), initrd->size);
    vmm_init();
    idt_enable();
    cpu_init();
    rtc_init();
    slab_init();
    thread_init(&kidle);
}
#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "misc/util.h"
#include "misc/mem.h"
#include "misc/printf.h"
#include "arch/gdt.h"
#include "arch/tss.h"
#include "arch/idt.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "userspace/cpu.h"
#include "multi/thread.h"
#include "multi/process.h"
#include "fs/tar.h"

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

void kidle() {
    threads_enabled = true;

    struct limine_file *initrd = mod_request.response->modules[0];

    tar_init(initrd->address, initrd->size);

    uint64_t init_size = 0;
    void *init_elf = tar_find("init.elf", &init_size);

    if (!init_elf)
        panic("main.c: kidle() -> init.elf not found\n");

    proc_create(init_elf, init_size);

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

    gdt_init();
    idt_init();
    tss_init();
    gdt_load_tss();
    pmm_init(mmap_request.response, exec_addr_request.response->physical_base);
    vmm_init();
    idt_enable();
    cpu_init();
    slab_init();
    thread_init(&kidle);
}
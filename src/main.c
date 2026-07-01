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
#include "fs/dev_vfs.h"
#include "fs/dev/console.h"
#include "fs/dev/fb.h"
#include "fb/fb.h"
#include "fb/font.h"
#include "fb/fbcon.h"

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

__attribute__((used, section(".limine_framebuffer_request")))
static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

struct limine_file *initrd;
struct limine_framebuffer *fb;

void kidle() {
    threads_enabled = true;

    fb_clear(0xff000000);
    
    tar_init(initrd->address, initrd->size);
    tarfs_init();

    fsnode_t *font_node = vfs_resolve("0:/etc/Lat2-Terminus16.psf");

    if (!font_node) {
        fb_clear(0xffff0000);

        for (;;)
            __asm__ volatile ("hlt");
    }

    uint64_t font_size = font_node->size;
    void *font_buf = kmalloc(font_size);
    int fn = font_node->ops->read(font_node, font_buf, font_size, 0);

    if (fn < 0 || (uint64_t)fn != font_size) {
        fb_clear(0xff00ffff);
        
        for (;;)
            __asm__ volatile ("hlt");
    }

    if (font_load_psf(font_buf, font_size, &curr_font) != 0) {
        fb_clear(0xff00ff00);

        for (;;)
            __asm__ volatile ("hlt");
    }

    devfs_init();
    console_dev_init();
    fb_dev_init();

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
    
    if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1)
        panic("main.c: kmain() -> no framebuffer found\n");
    
    if (fb_request.response->framebuffers[0]->bpp != 32)
        panic("main.c: kmain() -> 32-bit color not supported\n");
    
    hhdm_offset = hhdm_request.response->offset;
    initrd = mod_request.response->modules[0];
    fb = fb_request.response->framebuffers[0];

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
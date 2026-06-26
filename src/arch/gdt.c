#include "gdt.h"
#include "tss.h"
#include "../misc/printf.h"

#define GDT_ENTRIES 9

static gdt_entry_t gdt[GDT_ENTRIES];
static gdtr_t gdtr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low = base & 0xffff;
    gdt[i].base_mid = (base >> 16) & 0xff;
    gdt[i].base_high = (base >> 24) & 0xff;
    gdt[i].limit_low = limit & 0xffff;
    gdt[i].granularity = ((limit >> 16) & 0x0f) | (gran & 0xf0);
    gdt[i].access = access;
}

extern void gdt_flush(uint64_t gdtr_ptr);

void gdt_load_tss() {
    uint64_t base  = (uint64_t)&tss;
    uint32_t limit = sizeof(tss_t) - 1;
    gdt_tss_entry_t *tss_entry = (gdt_tss_entry_t *)&gdt[5];

    tss_entry->limit_low = limit & 0xffff;
    tss_entry->base_low = base & 0xffff;
    tss_entry->base_mid = (base >> 16) & 0xff;
    tss_entry->access = 0x89;
    tss_entry->granularity = (limit >> 16) & 0x0f;
    tss_entry->base_high = (base >> 24) & 0xff;
    tss_entry->base_upper = (base >> 32) & 0xffffffff;
    tss_entry->reserved1 = 0;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_flush((uint64_t)&gdtr);

    __asm__ volatile ("ltr %0" :: "r"((uint16_t)0x28));
}

void gdt_init(void) {
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_set_entry(0, 0, 0, 0x00, 0x00);
    gdt_set_entry(1, 0, 0xffffffff, 0x9a, 0x20);
    gdt_set_entry(2, 0, 0xffffffff, 0x92, 0xa0);
    gdt_set_entry(3, 0, 0xffffffff, 0xf2, 0xa0);
    gdt_set_entry(4, 0, 0xffffffff, 0xfa, 0x20);

    gdt_flush((uint64_t)&gdtr);
}
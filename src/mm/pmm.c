#include "pmm.h"

extern char __kernel_start[];
extern char __kernel_end[];

uintptr_t phys_kstart = 0;
uintptr_t phys_kend = 0;
uint64_t total_mem = 0;
uint64_t page_count = 0;
uint64_t *page_bitmap = NULL;

static size_t last_alloc_word = 0;

static inline size_t addr_to_bit(uintptr_t phys_addr) {
    return phys_addr / PAGE_SIZE;
}

static inline uintptr_t bit_to_addr(size_t bit) {
    return (uintptr_t)bit * PAGE_SIZE;
}

static inline size_t bit_to_word(size_t bit) {
    return bit / 64;
}

static inline size_t bit_to_offset(size_t bit) {
    return bit % 64;
}

void pmm_mark_used(uintptr_t phys_addr) {
    size_t bit  = addr_to_bit(phys_addr);

    if (bit >= page_count)
        panic("pmm.c: pmm_mark_used() -> addr %p out of range (bit %u >= %u)\n", phys_addr, bit, page_count);

    page_bitmap[bit_to_word(bit)] |= (1ULL << bit_to_offset(bit));
}

void pmm_mark_free(uintptr_t phys_addr) {
    size_t bit  = addr_to_bit(phys_addr);

    if (bit >= page_count)
        panic("pmm.c: pmm_mark_free() -> addr %p out of range (bit %u >= %u)\n", phys_addr, bit, page_count);

    page_bitmap[bit_to_word(bit)] &= ~(1ULL << bit_to_offset(bit));
}

void pmm_mark_range_used(uintptr_t base, size_t length) {
    uintptr_t end = base + length;

    for (uintptr_t addr = ALIGN_DOWN(base, PAGE_SIZE); addr < ALIGN_UP(end, PAGE_SIZE); addr += PAGE_SIZE)
        pmm_mark_used(addr);
}

void pmm_mark_range_free(uintptr_t base, size_t length) {
    uintptr_t end = base + length;
    for (uintptr_t addr = ALIGN_UP(base, PAGE_SIZE); addr < ALIGN_DOWN(end, PAGE_SIZE); addr += PAGE_SIZE)
        pmm_mark_free(addr);
}

uintptr_t pmm_alloc_page() {
    size_t words = (page_count + 63) / 64;

    for (size_t i = 0; i < words; i++) {
        size_t word = (last_alloc_word + i) % words;

        if (page_bitmap[word] == 0xffffffffffffffffULL)
            continue;
        
        size_t bit = __builtin_ctzll(~page_bitmap[word]);
        size_t global_bit = word * 64 + bit;

        if (global_bit >= page_count)
            continue;
        
        page_bitmap[word] |= (1ULL << bit);
        last_alloc_word = word;

        return bit_to_addr(global_bit);
    }

    panic("pmm.c: pmm_alloc_page() -> out of physical memory\n");

    return 0;
}

uintptr_t pmm_alloc_pages(size_t count) {
    if (count == 0)
        return 0;

    if (count == 1)
        return pmm_alloc_page();

    size_t run = 0;
    size_t run_start = 0;

    for (size_t bit = 0; bit < page_count; bit++) {
        size_t word   = bit_to_word(bit);
        size_t offset = bit_to_offset(bit);

        if (!(page_bitmap[word] & (1ULL << offset))) {
            if (run == 0)
                run_start = bit;

            run++;

            if (run == count) {
                for (size_t i = run_start; i < run_start + count; i++)
                    page_bitmap[bit_to_word(i)] |= (1ULL << bit_to_offset(i));

                return bit_to_addr(run_start);
            }
        } else
            run = 0;
    }

    panic("pmm.c: pmm_alloc_pages() -> failed to find %u contiguous pages\n", count);

    return 0;
}

void pmm_free_page(uintptr_t phys_addr) {
    if (phys_addr == 0)
        panic("pmm.c: pmm_free_page() -> attempt to free NULL\n");

    if (phys_addr % PAGE_SIZE != 0)
        panic("pmm.c: pmm_free_page() -> addr %p is not page-aligned\n", phys_addr);

    size_t bit = addr_to_bit(phys_addr);
    size_t word = bit_to_word(bit);
    size_t off = bit_to_offset(bit);

    if (bit >= page_count)
        panic("pmm.c: pmm_free_page() -> addr %p out of range\n", phys_addr);

    if (!(page_bitmap[word] & (1ULL << off)))
        panic("pmm.c: pmm_free_page() -> double free at %p\n", phys_addr);

    page_bitmap[word] &= ~(1ULL << off);
}

void pmm_free_pages(uintptr_t phys_addr, size_t count) {
    for (size_t i = 0; i < count; i++)
        pmm_free_page(phys_addr + i * PAGE_SIZE);
}

void pmm_init(struct limine_memmap_response *mmap, uint64_t kernel_phys_base, void *initrd_addr, uint64_t initrd_size) {
    uint64_t kernel_size = (uintptr_t)__kernel_end - (uintptr_t)__kernel_start;

    phys_kstart = kernel_phys_base;
    phys_kend = ALIGN_UP(kernel_phys_base + kernel_size, PAGE_SIZE);

    uintptr_t highest_addr = 0;

    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *entry = mmap->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t entry_end = entry->base + entry->length;

        if (entry_end > highest_addr)
            highest_addr = entry_end;
    }

    page_count = highest_addr / PAGE_SIZE;

    size_t bitmap_size = (page_count + 63) / 64 * sizeof(uint64_t);
    uintptr_t bitmap_phys = 0;
    bool bitmap_found = false;

    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *entry = mmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap_phys = entry->base;
            bitmap_found = true;

            break;
        }
    }

    if (!bitmap_found)
        panic("pmm.c: pmm() -> no room for bitmap\n");
    
    page_bitmap = (uint64_t *)(bitmap_phys + hhdm_offset);

    memset(page_bitmap, 0xff, bitmap_size);

    for (size_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *entry = mmap->entries[i];
        
        if (entry->type == LIMINE_MEMMAP_USABLE)
            pmm_mark_range_free(entry->base, entry->length);
    }

    pmm_mark_range_used(phys_kstart, phys_kend - phys_kstart);
    pmm_mark_range_used(bitmap_phys, bitmap_size);
    pmm_mark_range_used((uintptr_t)initrd_addr, initrd_size);
}
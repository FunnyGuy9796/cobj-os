#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"
#include "../misc/util.h"
#include "../misc/mem.h"
#include "../misc/printf.h"

#define PAGE_SIZE 4096ULL
#define ALIGN_UP(x, align) (((uintptr_t)(x) + (uintptr_t)(align) - 1u) & ~((uintptr_t)(align) - 1u))
#define ALIGN_DOWN(x, align) ((uintptr_t)(x) & ~((uintptr_t)(align) - 1u))

extern uintptr_t phys_kstart;
extern uintptr_t phys_kend;
extern uint64_t total_mem;
extern uint64_t page_count;
extern uint64_t *page_bitmap;

void pmm_mark_used(uintptr_t phys_addr);
void pmm_mark_free(uintptr_t phys_addr);
void pmm_mark_range_used(uintptr_t base, size_t length);
void pmm_mark_range_free(uintptr_t base, size_t length);
uintptr_t pmm_alloc_page(void);
uintptr_t pmm_alloc_pages(size_t count);
void pmm_free_page(uintptr_t phys_addr);
void pmm_free_pages(uintptr_t phys_addr, size_t count);
void pmm_init(struct limine_memmap_response *mmap, uint64_t kernel_phys_base, void *initrd_addr, uint64_t initrd_size);

#endif
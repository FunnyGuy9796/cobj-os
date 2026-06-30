#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include "../misc/util.h"
#include "pmm.h"
#include "heap.h"

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE (1ULL << 1)
#define PAGE_USER (1ULL << 2)
#define PAGE_PWT (1ULL << 3)
#define PAGE_PCD (1ULL << 4)
#define PAGE_ACCESSED (1ULL << 5)
#define PAGE_DIRTY (1ULL << 6)
#define PAGE_HUGE (1ULL << 7)
#define PAGE_NX (1ULL << 63)

#define VMM_FLAGS_KERNEL_RW (PAGE_PRESENT | PAGE_WRITE | PAGE_NX)
#define VMM_FLAGS_KERNEL_RX (PAGE_PRESENT)
#define VMM_FLAGS_USER_RW (PAGE_PRESENT | PAGE_WRITE | PAGE_USER | PAGE_NX)

typedef struct {
    uint64_t *pml4;
    uint64_t pml4_phys;
} addr_space_t;

extern addr_space_t kernel_addr_space;

void vmm_init();
uint64_t *vmm_get_pte(addr_space_t *space, uint64_t virt, int alloc);
int vmm_get_mapping(addr_space_t *space, uint64_t virt, uint64_t *phys_out, uint64_t *flags_out);
void vmm_map_page(addr_space_t *space, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(addr_space_t *space, uint64_t virt);
void vmm_switch_space(addr_space_t *space);
addr_space_t *vmm_create_user_space();

#endif
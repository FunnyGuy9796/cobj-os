#include "vmm.h"

#define PML4_IDX(virt) (((virt) >> 39) & 0x1ff)
#define PDPT_IDX(virt) (((virt) >> 30) & 0x1ff)
#define PD_IDX(virt) (((virt) >> 21) & 0x1ff)
#define PT_IDX(virt) (((virt) >> 12) & 0x1ff)

#define ENTRY_TO_PHYS(entry) ((entry) & ~0xfffULL)

addr_space_t kernel_addr_space;

static inline uint64_t *next_table(uint64_t entry) {
    return (uint64_t *)(ENTRY_TO_PHYS(entry) + hhdm_offset);
}

void vmm_init() {
    uint64_t phys = pmm_alloc_page();

    if (!phys)
        panic("vmm.c: vmm_init() -> phys is NULL\n");

    kernel_addr_space.pml4_phys = phys;
    kernel_addr_space.pml4 = (uint64_t *)(phys + hhdm_offset);

    memset(kernel_addr_space.pml4, 0, 4096);

    uint64_t cr3;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));

    uint64_t *limine_pml4 = (uint64_t *)((cr3 & ~0xfffULL) + hhdm_offset);

    for (int i = 256; i < 512; i++)
        kernel_addr_space.pml4[i] = limine_pml4[i];
    
    __asm__ volatile ("mfence" ::: "memory");
    __asm__ volatile ("mov %0, %%cr3" :: "r"(phys) : "memory");
}

uint64_t *vmm_get_pte(addr_space_t *space, uint64_t virt, int alloc) {
    uint64_t *pml4 = space->pml4;
    uint64_t *pml4e = &pml4[PML4_IDX(virt)];

    if (!(*pml4e & PAGE_PRESENT)) {
        if (!alloc)
            return NULL;
        
        uint64_t phys = pmm_alloc_page();

        if (!phys)
            panic("vmm.c: vmm_get_pte() -> out of memory allocating PDPT\n");
        
        uint64_t *pdpt = (uint64_t *)(phys + hhdm_offset);

        memset(pdpt, 0, 4096);

        *pml4e = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    uint64_t *pdpt = next_table(*pml4e);
    uint64_t *pdpte = &pdpt[PDPT_IDX(virt)];

    if (!(*pdpte & PAGE_PRESENT)) {
        if (!alloc)
            return NULL;
        
        uint64_t phys = pmm_alloc_page();

        if (!phys)
            panic("vmm.c: vmm_get_pte() -> out of memory allocating PD\n");

        uint64_t *pd = (uint64_t *)(phys + hhdm_offset);

        memset(pd, 0, 4096);

        *pdpte = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    uint64_t *pd = next_table(*pdpte);
    uint64_t *pde = &pd[PD_IDX(virt)];

    if (!(*pde & PAGE_PRESENT)) {
        if (!alloc)
            return NULL;
        
        uint64_t phys = pmm_alloc_page();

        if (!phys)
            panic("vmm.c: vmm_get_pte() -> out of memory allocating PT\n");
        
        uint64_t *pt = (uint64_t *)(phys + hhdm_offset);

        memset(pt, 0, 4096);

        *pde = phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    uint64_t *pt = next_table(*pde);

    return &pt[PT_IDX(virt)];
}

void vmm_map_page(addr_space_t *space, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t *pte = vmm_get_pte(space, virt, 1);

    if (!pte)
        panic("vmm.c: vmm_map_page() -> walk returned NULL during map\n");
    
    *pte = (phys & ~0xfffULL) | (flags | PAGE_PRESENT);

    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_unmap_page(addr_space_t *space, uint64_t virt) {
    uint64_t *pte = vmm_get_pte(space, virt, 0);

    if (!pte || !(*pte & PAGE_PRESENT))
        panic("vmm.c: vmm_unmap_page() -> unmapping a page that isn't mapped\n");
    
    *pte = 0;

    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_switch_space(addr_space_t *space) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(space->pml4_phys) : "memory");
}

addr_space_t *vmm_create_user_space() {
    addr_space_t *space = kmalloc(sizeof(addr_space_t));

    if (!space)
        panic("vmm.c: vmm_create_user_space() -> failed to allocate space\n");
    
    uint64_t phys = pmm_alloc_page();

    if (!phys)
        panic("vmm.c: vmm_create_user_space() -> failed to allocate PML4\n");
    
    space->pml4_phys = phys;
    space->pml4 = (uint64_t *)(phys + hhdm_offset);

    memset(space->pml4, 0, PAGE_SIZE);

    for (int i = 256; i < 512; i++)
        space->pml4[i] = kernel_addr_space.pml4[i];
    
    return space;
}
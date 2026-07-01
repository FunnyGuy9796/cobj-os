#include "elf.h"
#include "../mm/pmm.h"
#include "../misc/mem.h"

uint64_t elf_load(addr_space_t *space, const uint8_t *data, uint64_t size) {
    if (size < sizeof(elf64_hdr_t))
        return 0;
    
    elf64_hdr_t *hdr = (elf64_hdr_t *)data;

    if (hdr->magic != ELF_MAGIC) {
        fbcon_printf("[E] elf.c: elf_load() -> bad magic\n");

        return 0;
    }

    if (hdr->bits != 2) {
        fbcon_printf("[E] elf.c: elf_load() -> not 64-bit\n");

        return 0;
    }

    if (hdr->machine != EM_X86_64) {
        fbcon_printf("[E] elf.c: elf_load() -> not x86-64\n");

        return 0;
    }

    if (hdr->type != ET_EXEC) {
        fbcon_printf("[E] elf.c: elf_load() -> not an executable\n");

        return 0;
    }

    for (int i = 0; i < hdr->phnum; i++) {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(data + hdr->phoff + i * hdr->phentsize);

        if (phdr->type != PT_LOAD)
            continue;
        
        uint64_t vaddr_start = phdr->vaddr & ~0xfffULL;
        uint64_t vaddr_end = (phdr->vaddr + phdr->memsz + 0xfff) & ~0xfffULL;
        uint64_t vmm_flags = PAGE_PRESENT | PAGE_USER;

        if (phdr->flags & PF_W)
            vmm_flags |= PAGE_WRITE;
        
        for (uint64_t va = vaddr_start; va < vaddr_end; va += 4096) {
            uint64_t phys = pmm_alloc_page();

            if (!phys) {
                fbcon_printf("[E] elf.c: elf_load() -> out of memory\n");

                return 0;
            }

            memset((void *)(phys + hhdm_offset), 0, 4096);

            if (va < phdr->vaddr + phdr->filesz) {
                uint64_t file_offset = phdr->offset + (va - phdr->vaddr);

                if (va < phdr->vaddr)
                    file_offset = phdr->offset;
                
                uint64_t copy_offset = (phdr->vaddr > va) ? (phdr->vaddr - va) : 0;
                uint64_t copy_size = 4096 - copy_offset;
                uint64_t remaining = (phdr->vaddr + phdr->filesz) - va;

                if (copy_size > remaining)
                    copy_size = remaining;
                
                memcpy((void *)(phys + hhdm_offset + copy_offset), data + file_offset, copy_size);
            }

            vmm_map_page(space, va, phys, vmm_flags);
        }
    }

    return hdr->entry;
}
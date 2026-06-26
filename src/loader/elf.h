#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include "../mm/vmm.h"

#define ELF_MAGIC 0x464c457f
#define ET_EXEC 2
#define EM_X86_64 62
#define PT_LOAD 1

typedef struct {
    uint32_t magic;
    uint8_t  bits;
    uint8_t  endian;
    uint8_t  elf_version;
    uint8_t  os_abi;
    uint8_t  padding[8];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed)) elf64_hdr_t;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
} __attribute__((packed)) elf64_phdr_t;

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

uint64_t elf_load(addr_space_t *space, const uint8_t *data, uint64_t size);

#endif
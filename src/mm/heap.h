#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include "../misc/mem.h"
#include "../misc/printf.h"
#include "pmm.h"

#define SLAB_SIZES { 16, 32, 64, 128, 256, 512, 1024, 2048 }
#define SLAB_CLASS_COUNT 8

typedef struct slab_block {
    struct slab_block *next;
} slab_block_t;

typedef struct {
    slab_block_t *free;
    uint64_t obj_size;
    uint64_t total;
    uint64_t used;
} slab_cache_t;

typedef struct {
    uint32_t cache_idx;
    uint32_t _pad;
} slab_header_t;

void slab_init();
void *kmalloc(uint64_t size);
void kfree(void *ptr);

#endif
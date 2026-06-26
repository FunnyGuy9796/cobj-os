#include "heap.h"

static slab_cache_t caches[SLAB_CLASS_COUNT];

void slab_init() {
    uint64_t sizes[] = SLAB_SIZES;

    for (int i = 0; i < SLAB_CLASS_COUNT; i++) {
        caches[i].obj_size = sizes[i];
        caches[i].free = NULL;
        caches[i].total = 0;
        caches[i].used = 0;
    }
}

static void slab_refill(slab_cache_t *cache) {
    uint64_t phys = pmm_alloc_page();

    if (!phys)
        panic("heap.c: slab_refill() -> out of memory\n");
    
    uint8_t *page = (uint8_t *)(phys + hhdm_offset);

    memset(page, 0, PAGE_SIZE);

    uint64_t block_size = cache->obj_size + sizeof(slab_header_t);
    uint64_t count = PAGE_SIZE / block_size;

    for (uint64_t i = 0; i < count; i++) {
        uint8_t *raw = page + i * block_size;
        slab_header_t *hdr = (slab_header_t *)raw;

        hdr->cache_idx = (uint32_t)(cache - caches);
        hdr->_pad = 0;

        slab_block_t *block = (slab_block_t *)(raw + sizeof(slab_header_t));

        block->next = cache->free;
        cache->free = block;
    }

    cache->total += count;
}

void *kmalloc(uint64_t size) {
    if (size == 0)
        return NULL;
    
    for (int i = 0; i < SLAB_CLASS_COUNT; i++) {
        if (size > caches[i].obj_size)
            continue;
        
        slab_cache_t *cache = &caches[i];

        if (!cache->free)
            slab_refill(cache);
        
        slab_block_t *block = cache->free;

        cache->free = block->next;
        cache->used++;

        return (void *)block;
    }

    uint64_t phys = pmm_alloc_page();

    if (!phys)
        panic("heap.c: kmalloc() -> out of memory for large allocator\n");

    uint8_t *raw = (uint8_t *)(phys + hhdm_offset);
    slab_header_t *hdr = (slab_header_t *)raw;

    hdr->cache_idx = SLAB_CLASS_COUNT;
    hdr->_pad = 0;

    return (void *)(raw + sizeof(slab_header_t));
}

void kfree(void *ptr) {
    if (!ptr)
        return;
    
    slab_header_t *hdr = (slab_header_t *)ptr - 1;

    if (hdr->cache_idx == SLAB_CLASS_COUNT) {
        pmm_free_pages((uint64_t)hdr - hhdm_offset, hdr->_pad);

        return;
    }

    if (hdr->cache_idx >= SLAB_CLASS_COUNT)
        panic("heap.c: kfree() -> invalid cache_idx, possible double free or corruption\n");
    
    slab_cache_t *cache = &caches[hdr->cache_idx];
    slab_block_t *block = (slab_block_t *)ptr;

    block->next = cache->free;
    cache->free = block;
    cache->used--;
}
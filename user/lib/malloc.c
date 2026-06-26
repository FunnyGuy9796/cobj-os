#include "malloc.h"

typedef struct block {
    uint64_t size;
    bool free;
    struct block *next;
} block_t;

#define HEADER_SIZE (sizeof(block_t))
#define MIN_SPLIT (HEADER_SIZE + 16)

static uint8_t *heap_ptr = NULL;
static uint8_t *heap_end = NULL;
static block_t *head = NULL;

void *mmap(uint64_t addr, uint64_t size) {
    void *ret;

    __asm__ volatile (
        "mov $2, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"(addr), "S"(size)
        : "rcx", "r11", "memory"
    );

    return ret;
}

int munmap(void *addr, uint64_t size) {
    int ret;

    __asm__ volatile (
        "mov $3, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        : "D"((uint64_t)addr), "S"(size)
        : "rcx", "r11", "memory"
    );

    return ret;
}

static void *sbrk(uint64_t size) {
    if (heap_ptr == NULL || heap_ptr + size > heap_end) {
        void *page = mmap(0, size + 4096);

        heap_ptr = page;
        heap_end = page + size + 4096;
    }

    void *ptr = heap_ptr;

    heap_ptr += size;

    return ptr;
}

static block_t *extend_heap(uint64_t size) {
    uint64_t total = HEADER_SIZE + size;
    block_t *blk = (block_t *)sbrk(total);

    if (!blk)
        return NULL;
    
    blk->size = size;
    blk->free = false;
    blk->next = NULL;

    if (!head)
        head = blk;
    else {
        block_t *curr = head;

        while (curr->next)
            curr = curr->next;
        
        curr->next = blk;
    }

    return blk;
}

static void coalesce(block_t *blk) {
    while (blk->next && blk->next->free) {
        blk->size += HEADER_SIZE + blk->next->size;
        blk->next = blk->next->next;
    }
}

static void split(block_t *blk, uint64_t size) {
    if (blk->size < size + MIN_SPLIT)
        return;
    
    block_t *new_blk = (block_t *)((uint8_t *)(blk + 1) + size);

    new_blk->size = blk->size - size - HEADER_SIZE;
    new_blk->free = true;
    new_blk->next = blk->next;

    blk->size = size;
    blk->next = new_blk;
}

void *malloc(uint64_t size) {
    if (size == 0)
        return NULL;

    size = (size + 7) & ~(uint64_t)7;

    block_t *cur = head;

    while (cur) {
        if (cur->free && cur->size >= size) {
            coalesce(cur);
            
            if (cur->size >= size) {
                split(cur, size);

                cur->free = false;

                return (void *)(cur + 1);
            }
        }

        cur = cur->next;
    }

    block_t *blk = extend_heap(size);

    if (!blk)
        return NULL;

    return (void *)(blk + 1);
}

void free(void *ptr) {
    if (!ptr)
        return;

    block_t *blk = (block_t *)ptr - 1;

    blk->free = true;

    coalesce(blk);
}

void *realloc(void *ptr, uint64_t size) {
    if (!ptr)
        return malloc(size);

    if (size == 0) {
        free(ptr);
        
        return NULL;
    }

    block_t *blk = (block_t *)ptr - 1;

    size = (size + 7) & ~(uint64_t)7;

    if (blk->size >= size) {
        split(blk, size);
        
        return ptr;
    }

    coalesce(blk);

    if (blk->size >= size) {
        split(blk, size);

        return ptr;
    }

    void *new_ptr = malloc(size);

    if (!new_ptr)
        return NULL;

    uint64_t copy_size = blk->size < size ? blk->size : size;

    for (uint64_t i = 0; i < copy_size; i++)
        ((uint8_t *)new_ptr)[i] = ((uint8_t *)ptr)[i];

    free(ptr);

    return new_ptr;
}

void *calloc(uint64_t nmemb, uint64_t size) {
    uint64_t total = nmemb * size;
    void *ptr = malloc(total);

    if (ptr)
        for (uint64_t i = 0; i < total; i++) ((uint8_t *)ptr)[i] = 0;

    return ptr;
}
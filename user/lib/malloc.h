#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void *mmap(uint64_t addr, uint64_t size);
int munmap(void *addr, uint64_t size);
void *malloc(uint64_t size);
void free(void *ptr);
void *realloc(void *ptr, uint64_t size);
void *calloc(uint64_t nmemb, uint64_t size);

#endif
#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);
void *memset32(void *dst, int c, size_t n);
void *memcpy32(void *dst, const void *src, size_t n);
void *memmove32(void *dst, const void *src, size_t n);
int memcmp32(const void *a, const void *b, size_t n);

#endif
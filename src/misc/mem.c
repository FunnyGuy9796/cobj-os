#include "mem.h"

#define IS_ALIGNED(p) (((uintptr_t)(p) & 3) == 0)

void *memset(void *dst, int c, size_t n) {
    uint8_t *p = (uint8_t *)dst;

    while (n--)
        *p++ = (uint8_t)c;
    
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    while (n--)
        *d++ = *s++;
    
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;

        while (n--)
            *--d = *--s;
    }

    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *p = (const uint8_t *)a;
    const uint8_t *q = (const uint8_t *)b;

    while (n--) {
        if (*p != *q)
            return *p - *q;
        
        p++;
        q++;
    }

    return 0;
}

void *memset32(void *dst, int c, size_t n) {
    uint8_t *p8 = (uint8_t *)dst;

    while (n > 0 && !IS_ALIGNED(p8)) {
        *p8++ = (uint8_t)c;
        n--;
    }

    if (n >= 4) {
        uint32_t *p32 = (uint32_t *)p8;
        uint32_t val = (uint8_t)c;
        val |= val << 8;
        val |= val << 16;

        while (n >= 4) {
            *p32++ = val;
            n -= 4;
        }

        p8 = (uint8_t *)p32;
    }

    while (n--)
        *p8++ = (uint8_t)c;

    return dst;
}

void *memcpy32(void *dst, const void *src, size_t n) {
    uint8_t *d8 = (uint8_t *)dst;
    const uint8_t *s8 = (const uint8_t *)src;

    while (n > 0 && !IS_ALIGNED(d8)) {
        *d8++ = *s8++;
        n--;
    }

    if (n >= 4 && IS_ALIGNED(s8)) {
        uint32_t *d32 = (uint32_t *)d8;
        const uint32_t *s32 = (const uint32_t *)s8;

        while (n >= 4) {
            *d32++ = *s32++;
            n -= 4;
        }

        d8 = (uint8_t *)d32;
        s8 = (const uint8_t *)s32;
    }

    while (n--)
        *d8++ = *s8++;

    return dst;
}

void *memmove32(void *dst, const void *src, size_t n) {
    uint8_t *d8 = (uint8_t *)dst;
    const uint8_t *s8 = (const uint8_t *)src;

    if (d8 == s8 || n == 0)
        return dst;

    if (d8 < s8) {
        while (n > 0 && !IS_ALIGNED(d8)) {
            *d8++ = *s8++;
            n--;
        }

        if (n >= 4 && IS_ALIGNED(s8)) {
            uint32_t *d32 = (uint32_t *)d8;
            const uint32_t *s32 = (const uint32_t *)s8;

            while (n >= 4) {
                *d32++ = *s32++;
                n -= 4;
            }

            d8 = (uint8_t *)d32;
            s8 = (const uint8_t *)s32;
        }

        while (n--)
            *d8++ = *s8++;
    } else {
        d8 += n;
        s8 += n;

        while (n > 0 && !IS_ALIGNED(d8)) {
            *--d8 = *--s8;
            n--;
        }

        if (n >= 4 && IS_ALIGNED(s8)) {
            uint32_t *d32 = (uint32_t *)d8;
            const uint32_t *s32 = (const uint32_t *)s8;

            while (n >= 4) {
                *--d32 = *--s32;
                n -= 4;
            }

            d8 = (uint8_t *)d32;
            s8 = (const uint8_t *)s32;
        }

        while (n--)
            *--d8 = *--s8;
    }

    return dst;
}

int memcmp32(const void *a, const void *b, size_t n) {
    const uint8_t *p8 = (const uint8_t *)a;
    const uint8_t *q8 = (const uint8_t *)b;

    while (n > 0 && (!IS_ALIGNED(p8) || !IS_ALIGNED(q8))) {
        if (*p8 != *q8)
            return *p8 - *q8;

        p8++;
        q8++;
        n--;
    }

    if (n >= 4) {
        const uint32_t *p32 = (const uint32_t *)p8;
        const uint32_t *q32 = (const uint32_t *)q8;

        while (n >= 4) {
            if (*p32 != *q32) {
                p8 = (const uint8_t *)p32;
                q8 = (const uint8_t *)q32;

                if (p8[0] != q8[0])
                    return p8[0] - q8[0];

                if (p8[1] != q8[1])
                    return p8[1] - q8[1];

                if (p8[2] != q8[2])
                    return p8[2] - q8[2];

                return p8[3] - q8[3];
            }

            p32++;
            q32++;
            n -= 4;
        }

        p8 = (const uint8_t *)p32;
        q8 = (const uint8_t *)q32;
    }

    while (n--) {
        if (*p8 != *q8)
            return *p8 - *q8;

        p8++;
        q8++;
    }

    return 0;
}
#ifndef FB_H
#define FB_H

#include <stdint.h>
#include <stddef.h>
#include "../limine.h"

extern struct limine_framebuffer *fb;

static inline void fb_put(size_t x, size_t y, uint32_t color) {
    uint32_t *fb_ptr = (uint32_t *)fb->address;
    size_t pix_per_row = fb->pitch / (fb->bpp / 8);

    fb_ptr[y * pix_per_row + x] = color;
}

void fb_clear(uint32_t color);

#endif
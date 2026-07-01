#include "fb.h"

void fb_clear(uint32_t color) {
    uint32_t *fb_ptr = (uint32_t *)fb->address;
    size_t pix_per_row = fb->pitch / (fb->bpp / 8);

    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++)
            fb_ptr[y * pix_per_row + x] = color;
    }
}
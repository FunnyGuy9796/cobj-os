#ifndef DEV_FB_H
#define DEV_FB_H

#include <stdint.h>
#include "../vfs.h"

typedef struct {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
} fb_info_t;

void fb_dev_init();

#endif
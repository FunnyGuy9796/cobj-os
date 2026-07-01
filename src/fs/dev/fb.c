#include "fb.h"
#include "../dev_vfs.h"
#include "../../fb/fb.h"
#include "../../mm/heap.h"

static int fb_read(fsnode_t *node, void *buf, size_t len, size_t off) {
    fb_info_t info = {
        .width = fb->width,
        .height = fb->height,
        .pitch = fb->pitch,
        .bpp = fb->bpp,
        .red_mask_size = fb->red_mask_size,
        .red_mask_shift = fb->red_mask_shift,
        .green_mask_size = fb->green_mask_size,
        .green_mask_shift = fb->green_mask_shift,
        .blue_mask_size = fb->blue_mask_size,
        .blue_mask_shift = fb->blue_mask_shift,
    };

    size_t info_size = sizeof(fb_info_t);

    if (off < info_size) {
        size_t to_copy = info_size - off;

        if (to_copy > len)
            to_copy = len;
        
        memcpy(buf, (uint8_t *)&info + off, to_copy);

        return (int)to_copy;
    }

    size_t fb_size = fb->pitch * fb->height;
    size_t fb_off = off - info_size;

    if (fb_off >= fb_size)
        return 0;
    
    size_t to_copy = fb_size - fb_off;

    if (to_copy > len)
        to_copy = len;
    
    memcpy(buf, (uint8_t *)fb->address + fb_off, to_copy);

    return (int)to_copy;
}

static int fb_write(fsnode_t *node, const void *buf, size_t len, size_t off) {
    size_t info_size = sizeof(fb_info_t);

    if (off < info_size)
        return -1;

    size_t fb_off  = off - info_size;
    size_t fb_size = fb->pitch * fb->height;

    if (fb_off >= fb_size)
        return 0;

    size_t to_write = fb_size - fb_off;

    if (to_write > len)
        to_write = len;

    memcpy((uint8_t *)fb->address + fb_off, buf, to_write);

    return (int)to_write;
}

static fsnode_ops_t fb_ops = {
    .lookup = NULL,
    .readdir = NULL,
    .read = fb_read,
    .write = fb_write,
    .release = NULL,
};

void fb_dev_init() {
    devfs_register("fb", &fb_ops, NULL);
}
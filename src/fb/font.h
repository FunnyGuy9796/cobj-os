#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t glyph_size;
    uint32_t num_glyphs;
    const uint8_t *glyphs;
} font_t;

extern font_t curr_font;

int font_load_psf(const uint8_t *data, size_t size, font_t *out);

#endif
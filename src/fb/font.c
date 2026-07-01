#include "font.h"

#define PSF1_MAGIC 0x0436
#define PSF2_MAGIC 0x864ab572

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t mode;
    uint8_t charsize;
} psf1_header_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} psf2_header_t;

int font_load_psf(const uint8_t *data, size_t size, font_t *out) {
    if (size >= sizeof(psf2_header_t)) {
        const psf2_header_t *h2 = (const psf2_header_t *)data;

        if (h2->magic == PSF2_MAGIC) {
            out->width = h2->width;
            out->height = h2->height;
            out->glyph_size = h2->bytesperglyph;
            out->num_glyphs = h2->numglyph;
            out->glyphs = data + h2->headersize;

            return 0;
        }
    }

    if (size >= sizeof(psf1_header_t)) {
        const psf1_header_t *h1 = (const psf1_header_t *)data;

        if (h1->magic == PSF1_MAGIC) {
            out->width = 8;
            out->height = h1->charsize;
            out->glyph_size = h1->charsize;
            out->num_glyphs = (h1->mode & 0x01) ? 512 : 256;
            out->glyphs = data + sizeof(psf1_header_t);

            return 0;
        }
    }
    
    return -1;
}
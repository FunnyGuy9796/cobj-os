#include "fbcon.h"
#include "fb.h"
#include "font.h"
#include "../misc/mem.h"

font_t curr_font;
uint32_t bg_color = 0xff000000;
uint32_t fg_color = 0xffffffff;

static size_t cx = 0;
static size_t cy = 0;

static void fbcon_scroll() {
    size_t row_bytes = fb->pitch * curr_font.height;
    size_t total_bytes = fb->pitch * fb->height;
    uint8_t *base = (uint8_t *)fb->address;

    memmove(base, base + row_bytes, total_bytes - row_bytes);
    
    uint32_t *bottom = (uint32_t *)(base + (total_bytes - row_bytes));
    size_t pixels = row_bytes / sizeof(uint32_t);
    
    for (size_t i = 0; i < pixels; i++)
        bottom[i] = bg_color;
}

static void fbcon_newline() {
    cx = 0;
    cy++;

    if (cy * curr_font.height >= fb->height) {
        fbcon_scroll();

        cy--;
    }
}

static void fbcon_clear_cursor() {
    size_t ox = cx * curr_font.width;
    size_t oy = cy * curr_font.height;

    for (size_t row = 0; row < curr_font.height; row++) {
        for (size_t col = 0; col < curr_font.width; col++)
            fb_put(ox + col, oy + row, bg_color);
    }
}

static void fbcon_draw_cursor() {
    size_t ox = cx * curr_font.width;
    size_t oy = cy * curr_font.height;

    for (size_t row = 0; row < curr_font.height; row++) {
        for (size_t col = 0; col < curr_font.width; col++)
            fb_put(ox + col, oy + row, fg_color);
    }
}

void fbcon_putchar(char c) {
    fbcon_clear_cursor();

    if (c == '\n') {
        fbcon_newline();
        fbcon_draw_cursor();
        
        return;
    }

    if (c == '\r') {
        cx = 0;
        
        return;
    }

    if (c == '\b') {
        if (cx > 0)
            cx--;
        else if (cy > 0) {
            cy--;
            cx = (fb->width / curr_font.width) - 1;
        } else
            return;

        size_t ox = cx * curr_font.width;
        size_t oy = cy * curr_font.height;

        for (size_t row = 0; row < curr_font.height; row++) {
            for (size_t col = 0; col < curr_font.width; col++)
                fb_put(ox + col, oy + row, bg_color);
        }

        fbcon_draw_cursor();

        return;
    }

    size_t idx = (uint8_t)c;

    if (idx >= curr_font.num_glyphs) 
        idx = 0;

    const uint8_t *glyph = curr_font.glyphs + idx * curr_font.glyph_size;
    size_t bytes_per_row = (curr_font.width + 7) / 8;
    size_t ox = cx * curr_font.width;
    size_t oy = cy * curr_font.height;

    for (size_t row = 0; row < curr_font.height; row++) {
        const uint8_t *row_data = glyph + row * bytes_per_row;

        for (size_t col = 0; col < curr_font.width; col++) {
            uint8_t bit = row_data[col / 8] & (0x80 >> (col % 8));

            fb_put(ox + col, oy + row, bit ? fg_color : bg_color);
        }
    }

    cx++;

    if (cx * curr_font.width >= fb->width)
        fbcon_newline();
    
    fbcon_draw_cursor();
}

void fbcon_write(const char *str) {
    while (*str)
        fbcon_putchar(*str++);
}

void fbcon_clear() {
    fb_clear(bg_color);

    cx = 0;
    cy = 0;
}
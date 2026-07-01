#include <printf.h>
#include <mem.h>
#include <malloc.h>
#include <fs.h>

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

void fb_putpixel(int fd, fb_info_t *info, uint32_t x, uint32_t y, uint32_t color) {
    size_t off = sizeof(fb_info_t) + y * info->pitch + x * (info->bpp / 8);

    seek(fd, (int64_t)off, 0);
    write(fd, &color, info->bpp / 8);
}

int main(int argc, char **argv) {
    int fd = open("1:/fb");
    
    if (fd < 0) {
        printf("failed to open framebuffer device\n");

        return 1;
    }

    fb_info_t *info = malloc(sizeof(fb_info_t));
    int64_t n = read(fd, info, sizeof(fb_info_t));

    if (n <= 0) {
        printf("failed to get fb_info\n");
        close(fd);

        return 1;
    }

    printf("test: %llux%llu %ubpp\n", info->width, info->height, info->bpp);

    seek(fd, sizeof(fb_info_t), 0);

    for (int x = 0; x < 100; x++) {
        for (int y = 0; y < 100; y++)
            fb_putpixel(fd, info, x, y, 0xffff0000);
    }

    close(fd);

    return 0;
}
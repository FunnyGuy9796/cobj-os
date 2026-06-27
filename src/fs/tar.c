#include "tar.h"
#include "../misc/printf.h"
#include "../misc/mem.h"

static uint8_t *tar_data = NULL;
static uint64_t tar_size = 0;

static uint64_t tar_parse_octal(const char *s, int len) {
    uint64_t val = 0;

    for (int i = 0; i < len; i++) {
        if (s[i] < '0' || s[i] > '7')
            break;
        
        val = val * 8 + (s[i] - '0');
    }

    return val;
}

void tar_init(void *data, uint64_t size) {
    tar_data = (uint8_t *)data;
    tar_size = size;
}

void *tar_find(const char *name, uint64_t *size_out) {
    uint64_t offset = 0;

    while (offset + 512 <= tar_size) {
        tar_header_t *hdr = (tar_header_t *)(tar_data + offset);

        if (hdr->name[0] == '\0')
            break;
        
        uint64_t file_size = tar_parse_octal(hdr->size, 12);

        if (hdr->type == '0' || hdr->type == '\0') {
            if (strcmp(hdr->name, name) == 0) {
                if (size_out)
                    *size_out = file_size;

                return (void *)(tar_data + offset + 512);
            }
        }

        offset += 512 + ((file_size + 511) & ~511ULL);
    }

    serial_printf("[E] tar.c: tar_find() -> '%s' not found\n", name);

    return NULL;
}

int tar_listdir(const char *path, tar_entry_t *entries, int max) {
    uint64_t offset = 0;
    int count = 0;
    int path_len = strlen(path);

    while (offset + 512 <= tar_size && count < max) {
        tar_header_t *hdr = (tar_header_t *)(tar_data + offset);

        if (hdr->name[0] == '\0')
            break;

        uint64_t file_size = tar_parse_octal(hdr->size, 12);

        if (path_len == 0 || strncmp(hdr->name, path, path_len) == 0) {
            const char *relative = hdr->name + path_len;

            if (*relative == '/')
                relative++;

            if (*relative != '\0') {
                const char *slash = strchr(relative, '/');

                if (!slash || slash == relative + strlen(relative) - 1) {
                    tar_entry_t *e = &entries[count++];

                    int len = strlen(relative);

                    if (len > 0 && relative[len - 1] == '/')
                        len--;

                    if (len > 99)
                        len = 99;

                    memcpy(e->name, relative, len);

                    e->name[len] = '\0';

                    e->type = (hdr->type == '5') ? '5' : '0';
                    e->size = file_size;
                }
            }
        }

        offset += 512 + ((file_size + 511) & ~511ULL);
    }

    return count;
}
#ifndef TAR_H
#define TAR_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed)) tar_header_t;

void tar_init(void *data, uint64_t size);
void *tar_find(const char *name, uint64_t *size_out);

#endif
#ifndef FS_H

#include <stdint.h>

typedef struct {
    const void *data;
    uint64_t size;
    uint64_t offset;
} file_t;

#endif
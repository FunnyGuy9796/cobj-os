#ifndef FS_H
#define FS_H

#include <stdint.h>

int open(const char *path);
int64_t read(int fd, void *buf, uint64_t size);
int close(int fd);

#endif
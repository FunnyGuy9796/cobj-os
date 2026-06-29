#ifndef FS_H
#define FS_H

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef struct {
    char name[100];
    uint8_t type;
    uint64_t size;
} tar_entry_t;

int open(const char *path);
int64_t read(int fd, void *buf, uint64_t size);
int64_t write(int fd, const void *buf, uint64_t size);
int close(int fd);
int listdir(const char *path, tar_entry_t *entries, int max);

#endif
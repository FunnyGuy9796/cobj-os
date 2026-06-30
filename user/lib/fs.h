#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

typedef enum {
    NODE_FILE,
    NODE_DIR,
    NODE_DEVICE
} fsnode_type_t;

typedef struct {
    char name[256];
    fsnode_type_t type;
    uint64_t size;
} dirent_t;

int open(const char *path);
int64_t read(int fd, void *buf, uint64_t size);
int64_t write(int fd, const void *buf, uint64_t size);
int close(int fd);
int listdir(const char *path, dirent_t *entries, int max);
int getcwd(char *path);
int setcwd(const char *path, size_t len);

#endif
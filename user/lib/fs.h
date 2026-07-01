#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

#define STDIN 0
#define STDOUT 1
#define MAX_DRIVES 8

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

typedef struct {
    uint8_t id;
    char label[64];
    uint8_t present;
} drive_info_t;

int join(char *out, size_t out_size, const char *cwd, const char *input);
int open(const char *path);
int64_t read(int fd, void *buf, uint64_t size);
int64_t write(int fd, const void *buf, uint64_t size);
int close(int fd);
int listdir(const char *path, dirent_t *entries, int max);
int listdrives(drive_info_t *buf, int max);
int getcwd(char *path);
int setcwd(const char *path, size_t len);
int64_t seek(int fd, int64_t offset, int whence);

#endif
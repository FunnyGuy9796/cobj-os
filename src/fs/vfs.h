#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_DRIVES 8
#define MAX_OPEN_FILES 256
#define MAX_NAME 256

#define VFS_READ (1 << 0)
#define VFS_WRITE (1 << 1)
#define VFS_EXECUTE (1 << 2)

typedef enum {
    NODE_FILE,
    NODE_DIR,
    NODE_DEVICE
} fsnode_type_t;

typedef struct fsnode fsnode_t;

typedef struct {
    char name[256];
    fsnode_type_t type;
    size_t size;
} dirent_t;

typedef struct {
    fsnode_t *(*lookup)(fsnode_t *dir, const char *name);
    int (*readdir)(fsnode_t *dir, uint32_t index, dirent_t *out);
    int (*read)(fsnode_t *node, void *buf, size_t len, size_t off);
    int (*write)(fsnode_t *node, const void *buf, size_t len, size_t off);
    void (*release)(fsnode_t *node);
} fsnode_ops_t;

typedef struct fsnode {
    fsnode_type_t type;
    fsnode_ops_t *ops;
    void *data;
    size_t size;
    uint32_t refcount;
} fsnode_t;

typedef struct {
    fsnode_t *root;
    char label[64];
    uint8_t present;
} drive_t;

typedef struct file {
    fsnode_t *node;
    size_t offset;
    int flags;
} file_t;

extern drive_t vfs_drives[MAX_DRIVES];
extern file_t *vfs_handles[MAX_OPEN_FILES];

fsnode_t *vfs_resolve(const char *path);
int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buf, size_t len);
int vfs_write(int fd, const void *buf, size_t len);
int vfs_close(int fd);
int vfs_readdir(int fd, uint32_t index, dirent_t *out);

#endif
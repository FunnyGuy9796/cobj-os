#ifndef DEV_VFS_H
#define DEV_VFS_H

#include "vfs.h"
#include "../mm/heap.h"
#include "../misc/printf.h"

#define MAX_DEVFS_ENTRIES 32

void devfs_init();
void devfs_register(const char *name, fsnode_ops_t *ops, void *data);
fsnode_t *devfs_get_root();

#endif
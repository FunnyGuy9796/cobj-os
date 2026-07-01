#ifndef TAR_VFS_H
#define TAR_VFS_H

#include "vfs.h"
#include "../mm/heap.h"
#include "../misc/printf.h"

void tarfs_init();
fsnode_t *tarfs_get_root();

#endif
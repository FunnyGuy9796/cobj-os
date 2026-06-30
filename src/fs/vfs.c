#include "vfs.h"
#include "../misc/util.h"

drive_t vfs_drives[MAX_DRIVES];
file_t *vfs_handles[MAX_OPEN_FILES];

static int parse_path(const char *path, int *drive_num, const char **rel_path) {
    const char *colon = path;

    while (*colon && *colon != ':')
        colon++;

    if (*colon == ':') {
        *drive_num = 0;

        for (const char *p = path; p < colon; p++) {
            if (*p < '0' || *p > '9')
                return -1;

            *drive_num = (*drive_num * 10) + (*p - '0');
        }

        *rel_path = colon + 1;
    } else {
        *drive_num = 0;
        *rel_path = path;
    }

    return 0;
}

static const char *next_component(const char *path, char *out) {
    while (*path == '/')
        path++;

    if (*path == '\0')
        return NULL;

    int i = 0;
    
    while (*path && *path != '/' && i < MAX_NAME - 1)
        out[i++] = *path++;

    out[i] = '\0';

    return path;
}

fsnode_t *vfs_resolve(const char *path) {
    int drive_num;
    const char *rel_path;

    if (parse_path(path, &drive_num, &rel_path) < 0)
        return NULL;

    if (drive_num < 0 || drive_num >= MAX_DRIVES)
        return NULL;

    if (!vfs_drives[drive_num].present)
        return NULL;

    fsnode_t *root = vfs_drives[drive_num].root;

    if (!root)
        return NULL;

    fsnode_t *cur = root;
    char component[MAX_NAME];
    const char *p = rel_path;

    while ((p = next_component(p, component)) != NULL) {
        if (cur->type != NODE_DIR) {
            if (cur != root && cur->ops && cur->ops->release)
                cur->ops->release(cur);
            
            return NULL;
        }

        if (!cur->ops || !cur->ops->lookup) {
            if (cur != root && cur->ops && cur->ops->release)
                cur->ops->release(cur);
            
            return NULL;
        }

        fsnode_t *next = cur->ops->lookup(cur, component);

        if (cur != root && cur->ops && cur->ops->release)
            cur->ops->release(cur);

        if (!next)
            return NULL;

        cur = next;
    }

    if (cur == root)
        root->refcount++;

    return cur;
}
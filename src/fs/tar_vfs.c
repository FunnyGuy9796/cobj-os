#include "tar_vfs.h"
#include "tar.h"

static fsnode_ops_t tar_ops;

static fsnode_t *tar_make_node(tar_header_t *hdr) {
    fsnode_t *node = kmalloc(sizeof(fsnode_t));

    node->type = (hdr->type == '5') ? NODE_DIR : NODE_FILE;
    node->ops = &tar_ops;
    node->data = hdr;
    node->size = tar_parse_octal(hdr->size, 12);
    node->refcount = 1;

    return node;
}

static fsnode_t *tar_lookup(fsnode_t *dir, const char *name) {
    char fullpath[256];
    tar_header_t *dirhdr = (tar_header_t *)dir->data;

    if (dirhdr) {
        char dirname[256];
        int len = strlen(dirhdr->name);

        if (len > 0 && dirhdr->name[len - 1] == '/')
            len--;

        memcpy(dirname, dirhdr->name, len);

        dirname[len] = '\0';

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, name);
    } else
        strncpy(fullpath, name, sizeof(fullpath));

    uint64_t offset = 0;

    while (offset + 512 <= tar_size) {
        tar_header_t *hdr = (tar_header_t *)(tar_data + offset);

        if (hdr->name[0] == '\0')
            break;

        uint64_t file_size = tar_parse_octal(hdr->size, 12);

        if (strcmp(hdr->name, fullpath) == 0 || (hdr->type == '5' && strncmp(hdr->name, fullpath, strlen(fullpath)) == 0
            && hdr->name[strlen(fullpath)] == '/' && hdr->name[strlen(fullpath)+1] == '\0'))
            return tar_make_node(hdr);

        offset += 512 + ((file_size + 511) & ~511ULL);
    }

    return NULL;
}

static int tar_readdir(fsnode_t *dir, uint32_t index, dirent_t *out) {
    tar_header_t *dirhdr = (tar_header_t *)dir->data;
    char path[256];
    
    if (dirhdr) {
        int len = strlen(dirhdr->name);

        if (len > 0 && dirhdr->name[len - 1] == '/')
            len--;

        memcpy(path, dirhdr->name, len);

        path[len] = '\0';
    } else
        path[0] = '\0';

    int path_len = strlen(path);
    uint64_t offset = 0;
    uint32_t seen = 0;

    while (offset + 512 <= tar_size) {
        tar_header_t *hdr = (tar_header_t *)(tar_data + offset);

        if (hdr->name[0] == '\0')
            break;

        uint64_t file_size = tar_parse_octal(hdr->size, 12);

        if (path_len == 0 || strncmp(hdr->name, path, path_len) == 0) {
            const char *relative = hdr->name + path_len;

            if (*relative == '/')
                relative++;

            if (*relative != '\0') {
                const char *slash = strchr(relative, '/');

                if (!slash || slash == relative + strlen(relative) - 1) {
                    if (seen == index) {
                        int len = strlen(relative);

                        if (len > 0 && relative[len-1] == '/')
                            len--;

                        if (len > 99)
                            len = 99;

                        memcpy(out->name, relative, len);

                        out->name[len] = '\0';
                        out->type = (hdr->type == '5') ? NODE_DIR : NODE_FILE;
                        out->size = file_size;

                        return 0;
                    }

                    seen++;
                }
            }
        }

        offset += 512 + ((file_size + 511) & ~511ULL);
    }

    return -1;
}

static int tar_read(fsnode_t *node, void *buf, size_t len, size_t off) {
    tar_header_t *hdr = (tar_header_t *)node->data;
    uint64_t file_size = node->size;
    uint8_t *file_data = (uint8_t *)hdr + 512;

    if (off >= file_size)
        return 0;
    size_t to_read = len;

    if (off + to_read > file_size)
        to_read = file_size - off;

    memcpy(buf, file_data + off, to_read);

    return (int)to_read;
}

static void tar_release(fsnode_t *node) {
    if (node->refcount > 0)
        node->refcount--;

    if (node->refcount == 0)
        kfree(node);
}

static fsnode_ops_t tar_ops = {
    .lookup = tar_lookup,
    .readdir = tar_readdir,
    .read = tar_read,
    .write = NULL,
    .release = tar_release
};

fsnode_t *tar_get_root(void) {
    fsnode_t *root = kmalloc(sizeof(fsnode_t));

    root->type = NODE_DIR;
    root->ops = &tar_ops;
    root->data = NULL;
    root->size = 0;
    root->refcount = 1;

    return root;
}
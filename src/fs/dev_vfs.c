#include "dev_vfs.h"

typedef struct {
    char name[32];
    fsnode_ops_t *ops;
    void *data;
    int present;
} devfs_entry_t;

static devfs_entry_t entries[MAX_DEVFS_ENTRIES];
static int entry_count = 0;
static fsnode_t *devfs_root = NULL;

static fsnode_t *devfs_lookup(fsnode_t *dir, const char *name) {
    for (int i = 0; i < entry_count; i++) {
        if (!entries[i].present)
            continue;

        if (strcmp(entries[i].name, name) == 0) {
            fsnode_t *node = kmalloc(sizeof(fsnode_t));

            node->type = NODE_DEVICE;
            node->ops = entries[i].ops;
            node->data = entries[i].data;
            node->size = 0;
            node->refcount = 1;

            return node;
        }
    }

    return NULL;
}

static int devfs_readdir(fsnode_t *dir, uint32_t index, dirent_t *out) {
    uint32_t seen = 0;

    for (int i = 0; i < entry_count; i++) {
        if (!entries[i].present)
            continue;

        if (seen == index) {
            strncpy(out->name, entries[i].name, sizeof(out->name) - 1);

            out->name[sizeof(out->name) - 1] = '\0';
            out->type = NODE_DEVICE;
            out->size = 0;

            return 0;
        }

        seen++;
    }

    return -1;
}

static void devfs_release(fsnode_t *node) {
    if (node == devfs_root)
        return;

    if (node->refcount > 0)
        node->refcount--;

    if (node->refcount == 0)
        kfree(node);
}

static fsnode_ops_t devfs_root_ops = {
    .lookup = devfs_lookup,
    .readdir = devfs_readdir,
    .read = NULL,
    .write = NULL,
    .release = devfs_release,
};

void devfs_init() {
    entry_count = 0;

    devfs_root = kmalloc(sizeof(fsnode_t));

    devfs_root->type = NODE_DIR;
    devfs_root->ops = &devfs_root_ops;
    devfs_root->data = NULL;
    devfs_root->size = 0;
    devfs_root->refcount = 1;

    vfs_drives[1].root = devfs_root;
    vfs_drives[1].present = 1;

    strncpy(vfs_drives[1].label, "dev", sizeof(vfs_drives[1].label) - 1);
}

void devfs_register(const char *name, fsnode_ops_t *ops, void *data) {
    if (entry_count >= MAX_DEVFS_ENTRIES) {
        serial_printf("[E] devfs: too many devices, could not register %s\n", name);

        return;
    }

    strncpy(entries[entry_count].name, name, sizeof(entries[entry_count].name) - 1);

    entries[entry_count].name[sizeof(entries[entry_count].name) - 1] = '\0';
    entries[entry_count].ops = ops;
    entries[entry_count].data = data;
    entries[entry_count].present = 1;

    entry_count++;
}

fsnode_t *devfs_get_root() {
    return devfs_root;
}
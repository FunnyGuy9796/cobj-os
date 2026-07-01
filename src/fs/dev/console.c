#include "console.h"
#include "../dev_vfs.h"
#include "../../misc/printf.h"
#include "../../mm/heap.h"
#include "../../devices/kb.h"

static int stdin_read(fsnode_t *node, void *buf, size_t len, size_t off) {
    char *cbuf = (char *)buf;

    for (size_t i = 0; i < len; i++) {
        cbuf[i] = kbd_getchar();

        if (cbuf[i] == '\n' || cbuf[i] == '\r')
            return (int)(i + 1);
    }

    return (int)len;
}

static fsnode_ops_t stdin_ops = {
    .lookup = NULL,
    .readdir = NULL,
    .read = stdin_read,
    .write = NULL,
    .release = NULL,
};

static int stdout_write(fsnode_t *node, const void *buf, size_t len, size_t off) {
    const char *cbuf = (const char *)buf;

    for (size_t i = 0; i < len; i++)
        fbcon_putchar(cbuf[i]);
    
    return (int)len;
}

static fsnode_ops_t stdout_ops = {
    .lookup = NULL,
    .readdir = NULL,
    .read = NULL,
    .write = stdout_write,
    .release = NULL,
};

static int null_read(fsnode_t *node, void *buf, size_t len, size_t off) {
    return 0;
}

static int null_write(fsnode_t *node, const void *buf, size_t len, size_t off) {
    return (int)len;
}

static fsnode_ops_t null_ops = {
    .lookup = NULL,
    .readdir = NULL,
    .read = null_read,
    .write = null_write,
    .release = NULL,
};

static int zero_read(fsnode_t *node, void *buf, size_t len, size_t off) {
    memset(buf, 0, len);

    return (int)len;
}

static fsnode_ops_t zero_ops = {
    .lookup = NULL,
    .readdir = NULL,
    .read = zero_read,
    .write = null_write,
    .release = NULL,
};

void console_dev_init() {
    devfs_register("stdin", &stdin_ops,  NULL);
    devfs_register("stdout", &stdout_ops, NULL);
    devfs_register("null", &null_ops,   NULL);
    devfs_register("zero", &zero_ops,   NULL);
}
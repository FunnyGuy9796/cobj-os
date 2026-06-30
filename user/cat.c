#include <printf.h>
#include <mem.h>
#include <fs.h>

static int path_join(char *out, size_t out_size, const char *cwd, const char *input) {
    char drive[8];
    const char *rest;

    if (strchr(input, ':')) {
        strncpy(out, input, out_size - 1);

        out[out_size - 1] = '\0';

        return 0;
    }

    const char *colon = strchr(cwd, ':');

    if (!colon)
        return -1;

    size_t drive_len = (size_t)(colon - cwd) + 1;

    if (drive_len >= sizeof(drive))
        return -1;

    memcpy(drive, cwd, drive_len);

    drive[drive_len] = '\0';

    char comps[32][64];
    int ncomps = 0;

    const char *p = colon + 1;
    char tok[64];
    int ti = 0;

    #define PUSH_TOK() do { \
        if (ti > 0) { \
            tok[ti] = '\0'; \
            if (strcmp(tok, ".") == 0) { \
                /* no-op */ \
            } else if (strcmp(tok, "..") == 0) { \
                if (ncomps > 0) ncomps--; \
            } else { \
                if (ncomps >= 32) return -1; \
                strncpy(comps[ncomps], tok, sizeof(comps[ncomps]) - 1); \
                comps[ncomps][sizeof(comps[ncomps]) - 1] = '\0'; \
                ncomps++; \
            } \
            ti = 0; \
        } \
    } while (0)

    if (input[0] != '/') {
        while (*p) {
            if (*p == '/') {
                PUSH_TOK();
            } else if (ti < (int)sizeof(tok) - 1) {
                tok[ti++] = *p;
            }
            p++;
        }
        PUSH_TOK();
    }

    p = input;
    ti = 0;

    while (*p) {
        if (*p == '/') {
            PUSH_TOK();
        } else if (ti < (int)sizeof(tok) - 1) {
            tok[ti++] = *p;
        }
        p++;
    }
    PUSH_TOK();

    #undef PUSH_TOK

    size_t off = 0;
    int n = snprintf(out, out_size, "%s/", drive);

    if (n < 0 || (size_t)n >= out_size)
        return -1;

    off = (size_t)n;

    for (int i = 0; i < ncomps; i++) {
        n = snprintf(out + off, out_size - off, "%s%s", comps[i], (i == ncomps - 1) ? "" : "/");

        if (n < 0 || (size_t)n >= out_size - off)
            return -1;

        off += (size_t)n;
    }

    return 0;
}

int main(int argc, char **argv) {
    char cwd[256];

    if (getcwd(cwd) < 0) {
        printf("ls: could not get cwd\n");

        return 1;
    }

    char target[256];
    const char *input = argc > 1 ? argv[1] : ".";

    if (path_join(target, sizeof(target), cwd, input) < 0) {
        printf("cat: %s: invalid path\n", input);

        return 1;
    }

    int fd = open(target);

    if (fd < 0) {
        printf("cat: %s: not found\n", target);

        return 1;
    }

    char buf[256];
    int64_t n;

    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';

        printf("%s", buf);
    }

    printf("\n");
    close(fd);

    return 0;
}
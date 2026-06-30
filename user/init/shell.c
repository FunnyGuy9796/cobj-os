#include <stddef.h>
#include <printf.h>
#include <util.h>
#include <mem.h>
#include <sys.h>
#include <proc.h>
#include <malloc.h>
#include <fs.h>

static char buf[256];

static void readline(char *out, int max) {
    int i = 0;

    while (i < max - 1) {
        char c;

        read(STDIN, &c, 1);

        if (c == '\r' || c == '\n') {
            printf("\n");

            break;
        }

        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;

                printf("\b \b");
            }

            continue;
        }

        out[i++] = c;
        
        char echo[2] = { c, 0 };

        printf("%s", echo);
    }

    out[i] = '\0';
}

static char *args[32];
static int arg_count;

static void parse_args(char *input) {
    arg_count = 0;

    char *p = input;

    while (*p) {
        while (*p == ' ')
            p++;

        if (*p == '\0')
            break;

        args[arg_count++] = p;

        while (*p && *p != ' ')
            p++;

        if (*p == ' ')
            *p++ = '\0';
    }
}

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

static char cwd[256];

int main(int argc, char **argv) {
    rtc_time_t t;

    gettime(&t);

    printf("Welcome to Cobj OS\n");
    printf("%04d-%02d-%02d %02d:%02d:%02d\n\n", t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    while (1) {
        if (getcwd(cwd) < 0) {
            printf("could not get CWD\n");

            return 1;
        }

        printf("cobj[%s]> ", cwd);
        readline(buf, sizeof(buf));

        if (buf[0] == '\0')
            continue;
        
        parse_args(buf);

        if (arg_count == 0)
            continue;
        
        if (strcmp(args[0], "cd") == 0) {
            if (arg_count < 2)
                printf("cd: missing argument\n");
            else {
                char candidate[256];

                if (path_join(candidate, sizeof(candidate), cwd, args[1]) < 0)
                    printf("cd: %s: invalid path\n", args[1]);
                else if (setcwd(candidate, strlen(candidate)) < 0)
                    printf("cd: %s: no such file or directory\n", args[1]);
                else {
                    strncpy(cwd, candidate, sizeof(cwd) - 1);

                    cwd[sizeof(cwd) - 1] = '\0';
                }
            }

            continue;
        }

        uint64_t ret = spawn(args[0], (const char **)args + 1, arg_count - 1);

        if (ret == (uint64_t)-1)
            printf("command not found...\n");
        else
            waitpid(ret);
    }

    return 0;
}
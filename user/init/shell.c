#include <stddef.h>
#include <printf.h>
#include <util.h>
#include <mem.h>
#include <sys.h>
#include <proc.h>
#include <malloc.h>
#include <fs.h>

static char bin_dir[256] = "0:/";
static char cwd[256];

static uint64_t spawn_search(const char *name, const char **argv, int argc) {
    char candidate[256];

    if (strncmp(name, "./", 2) == 0 || strncmp(name, "../", 3) == 0) {
        const char *rel = (name[1] == '/') ? name + 2 : name;
        
        if (join(candidate, sizeof(candidate), cwd, rel) < 0)
            return (uint64_t)-1;

        return spawn(candidate, argv, argc);
    }

    if (strchr(name, '/') || strchr(name, ':'))
        return spawn(name, argv, argc);

    snprintf(candidate, sizeof(candidate), "%s/%s", bin_dir, name);

    return spawn(candidate, argv, argc);
}

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

int main(int argc, char **argv) {
    clear();

    if (argv[1] && strcmp(argv[1], "--bindir") == 0) {
        if (argv[2])
            strncpy(bin_dir, argv[2], sizeof(bin_dir) - 1);
    }

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

                if (join(candidate, sizeof(candidate), cwd, args[1]) < 0)
                    printf("cd: %s: invalid path\n", args[1]);
                else if (setcwd(candidate, strlen(candidate)) < 0)
                    printf("cd: %s: no such file or directory\n", args[1]);
            }

            continue;
        } else if (strcmp(args[0], "bindir") == 0) {
            if (arg_count < 2)
                printf("bindir: missing argument\n");
            else {
                char candidate[256];

                if (join(candidate, sizeof(candidate), cwd, args[1]) < 0)
                    printf("bindir: %s: invalid path\n", args[1]);
                else {
                    strncpy(bin_dir, candidate, sizeof(bin_dir) - 1);

                    bin_dir[sizeof(bin_dir) - 1] = '\0';
                }
            }

            continue;
        }

        uint64_t ret = spawn_search(args[0], (const char **)args + 1, arg_count - 1);

        if (ret == (uint64_t)-1)
            printf("command not found...\n");
        else
            waitpid(ret);
    }

    return 0;
}
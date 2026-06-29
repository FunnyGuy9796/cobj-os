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

static char cwd[256];

int main(int argc, char **argv) {
    rtc_time_t t;

    gettime(&t);

    printf("Welcome to Cobj OS\n");
    printf("%04d-%02d-%02d %02d:%02d:%02d\n\n", t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    cwd[0] = '/';
    cwd[1] = '\0';

    while (1) {
        printf("cobj:%s$> ", cwd);
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
                strncpy(cwd, args[1], sizeof(cwd) - 1);

                cwd[sizeof(cwd) - 1] = '\0';
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
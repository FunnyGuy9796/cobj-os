#include <stddef.h>
#include <printf.h>
#include <util.h>
#include <io.h>
#include <sys.h>
#include <proc.h>
#include <malloc.h>

static char buf[256];

static void readline(char *out, int max) {
    int i = 0;

    while (i < max - 1) {
        char c = read_char();

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
    rtc_time_t t;

    gettime(&t);

    printf("welcome to cobj OS\n");
    printf("%04d-%02d-%02d %02d:%02d:%02d\n\n", t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    while (1) {
        printf("cobj$> ");
        readline(buf, sizeof(buf));

        if (buf[0] == '\0')
            continue;
        
        parse_args(buf);

        if (arg_count == 0)
            continue;

        uint64_t ret = spawn(args[0], (const char **)args + 1, arg_count - 1);

        if (ret < 0)
            printf("command not found: %s\n", args[0]);
        else
            waitpid(ret);
    }

    return 0;
}
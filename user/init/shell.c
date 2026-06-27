#include <printf.h>
#include <io.h>
#include <sys.h>
#include <proc.h>

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

int main() {
    rtc_time_t t;

    gettime(&t);

    printf("welcome to cobj OS\n");
    printf("%04d-%02d-%02d %02d:%02d:%02d\n\n", t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    while (1) {
        printf("$> ");
        readline(buf, sizeof(buf));

        if (buf[0] == '\0')
            continue;

        if (strcmp(buf, "exit") == 0)
            break;

        uint64_t ret = spawn(buf);

        if (ret < 0)
            printf("command not found: %s\n", buf);
        else
            waitpid(ret);
    }

    return 0;
}
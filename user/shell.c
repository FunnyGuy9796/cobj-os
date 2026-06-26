#include <printf.h>
#include <io.h>
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
    printf("cobj shell\n");

    while (1) {
        printf("> ");
        readline(buf, sizeof(buf));

        if (buf[0] == '\0')
            continue;

        if (strcmp(buf, "exit") == 0)
            break;

        uint64_t ret = spawn(buf);

        if (ret < 0)
            printf("unknown command: %s\n", buf);
        else
            waitpid(ret);
    }

    return 0;
}
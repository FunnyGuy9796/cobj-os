#include <stddef.h>
#include <printf.h>
#include <fs.h>
#include <proc.h>
#include <port.h>

static char buf[512];

int main(int argc, char **argv) {
    printf("\033[2J\033[H");

    int fd = open("info.txt");

    if (fd < 0) {
        printf("failed to open info.txt\n");

        return 1;
    }

    int64_t n = read(fd, buf, sizeof(buf) - 1);

    if (n > 0) {
        buf[n] = '\0';

        printf("%s\n\n", buf);
    }

    close(fd);
    spawn("init/shell", NULL, 0);

    port_token_t port = create_port();

    printf("init.c: port: %lu\n", port);

    char token_str[32];

    ksnprintf(token_str, sizeof(token_str), "%lu", port);
    spawn("init/test", (const char *[]){ token_str }, 1);

    char msg[16];
    int response = receive_port(port, (void *)msg, 16);

    if (response == -1) {
        printf("failed to receive response\n");

        return 1;
    }

    printf("msg: %s\n", msg);

    return 0;
}
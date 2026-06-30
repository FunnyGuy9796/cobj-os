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

    return 0;
}
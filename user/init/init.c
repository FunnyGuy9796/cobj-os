#include <stddef.h>
#include <printf.h>
#include <fs.h>
#include <proc.h>
#include <port.h>

static char buf[512];

int main(int argc, char **argv) {
    int fd = open("0:/info.txt");

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

    const char *shell_args[] = {"--bindir", "0:/bin"};

    spawn("0:/init/shell", shell_args, 2);

    return 0;
}
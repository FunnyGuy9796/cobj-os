#include <printf.h>
#include <fs.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("usage: cat <filename>\n");

        return 1;
    }

    int fd = open(argv[1]);

    if (fd < 0) {
        printf("cat: %s: not found\n", argv[1]);

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
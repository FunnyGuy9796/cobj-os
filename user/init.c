#include <printf.h>
#include <malloc.h>
#include <proc.h>

int main() {
    printf("\033[2J\033[H");
    printf("hello from userspace\n");

    spawn("shell.elf");

    return 0;
}
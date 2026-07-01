#include <printf.h>
#include <fs.h>

int main(int argc, char **argv) {
    drive_info_t drives[MAX_DRIVES];
    int n = listdrives(drives, MAX_DRIVES);

    if (n <= 0) {
        printf("no drives found\n");
        return 1;
    }

    printf("ID   LABEL\n");
    printf("---- ----------------\n");

    for (int i = 0; i < n; i++)
        printf("%-4d %s\n", drives[i].id, drives[i].label);

    return 0;
}
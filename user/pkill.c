#include <printf.h>
#include <proc.h>
#include <util.h>

#define MAX_PROCS 64

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: pkill <name>\n");

        return 1;
    }

    proc_info_t procs[MAX_PROCS];
    int count = listprocs(procs, MAX_PROCS);
    int killed = 0;

    if (isdigit(argv[1])) {
        int pid = atoi(argv[1]);
        int ret = kill(pid);

        if (ret == 0) {
            printf("killed pid %d\n", pid);

            killed++;
        }
    } else {
        for (int i = 0; i < count; i++) {
            if (strcmp(procs[i].name, argv[1]) == 0) {
                int ret = kill(procs[i].pid);

                if (ret == 0) {
                    printf("killed %s (pid %d)\n", procs[i].name, procs[i].pid);

                    killed++;
                }
            }
        }
    }

    if (!killed)
        printf("no process named '%s'\n", argv[1]);

    return 0;
}
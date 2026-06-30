#include <printf.h>
#include <proc.h>

#define MAX_PROCS 64

int main() {
    proc_info_t procs[MAX_PROCS];
    int count = listprocs(procs, MAX_PROCS);

    printf("PID  STATE    NAME\n");
    printf("---- -------- ----------------\n");

    for (int i = 0; i < count; i++) {
        const char *state;
        switch (procs[i].state) {
            case 0: {
                state = "ready";
                
                break;
            }

            case 1: {
                state = "running";
                
                break;
            }

            case 2: {
                state = "blocked";
                
                break;
            }

            case 3: {
                state = "dead";
                
                break;
            }

            default: {
                state = "unknown";
                
                break;
            }
        }

        printf("%-4d %-8s %s\n", procs[i].pid, state, procs[i].name);
    }

    return 0;
}
#include <printf.h>
#include <sys.h>

int main() {
    uint64_t ticks = getuptime();

    printf("%d ticks (%d ms)\n", ticks, ticks * 10);

    return 0;
}
#include <printf.h>
#include <sys.h>

int main() {
    rtc_time_t t;

    gettime(&t);

    printf("%04d-%02d-%02d %02d:%02d:%02d\n", t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    return 0;
}
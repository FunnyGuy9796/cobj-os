#ifndef SYS_H
#define SYS_H

#include <stdint.h>

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

uint64_t getuptime();
void gettime(rtc_time_t *out);
int sleep(uint64_t ms);

#endif
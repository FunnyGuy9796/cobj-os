#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "../misc/util.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09
#define RTC_STATUS_A 0x0a
#define RTC_STATUS_B 0x0b

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

extern rtc_time_t boot_time;
extern uint64_t boot_epoch;
extern uint64_t boot_ticks;

rtc_time_t rtc_read();
void rtc_init();
uint64_t rtc_to_epoch(rtc_time_t *t);
rtc_time_t epoch_to_rtc(uint64_t epoch);
uint64_t rtc_now_epoch();
rtc_time_t rtc_now();

#endif
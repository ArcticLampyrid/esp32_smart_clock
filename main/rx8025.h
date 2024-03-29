#pragma once
#include "bcd.h"
#include <time.h>
typedef uint8_t rx8025_weekday_t;
#define WEEKDAY_SUNDAY ((rx8025_weekday_t)(1 << 0))
#define WEEKDAY_MONDAY ((rx8025_weekday_t)(1 << 1))
#define WEEKDAY_TUESDAY ((rx8025_weekday_t)(1 << 2))
#define WEEKDAY_WEDNESDAY ((rx8025_weekday_t)(1 << 3))
#define WEEKDAY_THURSDAY ((rx8025_weekday_t)(1 << 4))
#define WEEKDAY_FRIDAY ((rx8025_weekday_t)(1 << 5))
#define WEEKDAY_SATURDAY ((rx8025_weekday_t)(1 << 6))
struct rx8025_time_t
{
    bcd8_t year;
    bcd8_t month;
    bcd8_t day;
    rx8025_weekday_t weekday;
    bcd8_t hour;
    bcd8_t minute;
    bcd8_t second;
};
void rx8025_init();
void rx8025_set_time(struct rx8025_time_t time);
struct rx8025_time_t rx8025_get_time();
void rx8025_time_fix_weekday(struct rx8025_time_t *time);
int rx8025_time_cmp(struct rx8025_time_t x, struct rx8025_time_t y);
struct rx8025_time_t rx8025_time_next_day(struct rx8025_time_t time);
void rx8025_time_apply_up_operation(struct rx8025_time_t *time, int progress);
void rx8025_time_apply_down_operation(struct rx8025_time_t *time, int progress);
struct rx8025_time_t rx8025_time_from_tm(struct tm *it);
inline struct rx8025_time_t rx8025_time_min_value()
{
    return (struct rx8025_time_t){
        .year = 0x00,
        .month = 0x01,
        .day = 0x01,
        .weekday = WEEKDAY_SATURDAY,
        .hour = 0x00,
        .minute = 0x00,
        .second = 0x00,
    };
}
inline struct rx8025_time_t rx8025_time_max_value()
{
    return (struct rx8025_time_t){
        .year = 0x99,
        .month = 0x12,
        .day = 0x31,
        .weekday = WEEKDAY_THURSDAY,
        .hour = 0x23,
        .minute = 0x59,
        .second = 0x59,
    };
}
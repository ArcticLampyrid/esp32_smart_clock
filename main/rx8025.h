#pragma once
#include "bcd.h"
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
void rx8025_fix_weekday(struct rx8025_time_t* time);
#include "rx8025.h"
static struct rx8025_time_t mock = {
    .year = 0x22,
    .month = 0x11,
    .day = 0x07,
    .weekday = WEEKDAY_MONDAY,
    .hour = 0x09,
    .minute = 0x31,
    .second = 0x22,
};

static bcd8_t day_in_month_bcd(bcd8_t year2, bcd8_t month)
{
    static bcd8_t day_in_month[12] = {0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31};
    bcd8_t result = day_in_month[bcd8_to_uint8(month) - 1];
    if (month == 0x02 && bcd8_to_uint8(year2) % 4 == 0)
    {
        result += 1;
    }
    return result;
}

void rx8025_init()
{
    // TODO: implement
}
void rx8025_set_time(struct rx8025_time_t time)
{
    // TODO: implement
    mock = time;
}
struct rx8025_time_t rx8025_get_time()
{
    // TODO: implement
    return mock;
}
void rx8025_time_fix_weekday(struct rx8025_time_t *time)
{
    int d = bcd8_to_uint8(time->day);
    int m = bcd8_to_uint8(time->month);
    int y = 2000 + bcd8_to_uint8(time->year);
    int weekday = (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
    time->weekday = 1 << weekday;
}
int rx8025_time_cmp(struct rx8025_time_t x, struct rx8025_time_t y)
{
    int t = x.year - y.year;
    if (t == 0)
        t = x.month - y.month;
    if (t == 0)
        t = x.day - y.day;
    if (t == 0)
        t = x.hour - y.hour;
    if (t == 0)
        t = x.minute - y.minute;
    if (t == 0)
        t = x.second - y.second;
    return t;
}
struct rx8025_time_t rx8025_time_next_day(struct rx8025_time_t time)
{
    struct rx8025_time_t result = time;
    result.day++;
    if (result.day > day_in_month_bcd(result.year, result.month))
    {
        result.day = 1;
        result.month++;
        if (result.month > 12)
        {
            result.month = 1;
            result.year++;
        }
    }
    result.weekday <<= 1;
    if (result.weekday & 0x80)
    {
        result.weekday = 0x1;
    }
    return result;
}

void rx8025_time_apply_down_operation(struct rx8025_time_t *time, int progress)
{
    switch (progress)
    {
    case 0:
        time->year = time->year == 0x99 ? 0x00 : bcd8_inc(time->year);
        rx8025_time_fix_weekday(time);
        break;
    case 1:
        time->month = time->month == 0x12 ? 0x01 : bcd8_inc(time->month);
        rx8025_time_fix_weekday(time);
        break;
    case 2:
        if (time->day == day_in_month_bcd(time->year, time->month))
            time->day = 0x01;
        else
            time->day = bcd8_inc(time->day);
        rx8025_time_fix_weekday(time);
        break;
    case 3:
        time->hour = time->hour == 0x23 ? 0x00 : bcd8_inc(time->hour);
        break;
    case 4:
        time->minute = time->minute == 0x59 ? 0x00 : bcd8_inc(time->minute);
        break;
    case 5:
        time->second = time->second == 0x59 ? 0x00 : bcd8_inc(time->second);
        break;
    }
}

void rx8025_time_apply_up_operation(struct rx8025_time_t *time, int progress)
{
    switch (progress)
    {
    case 0:
        time->year = time->year == 0x00 ? 0x99 : bcd8_dec(time->year);
        rx8025_time_fix_weekday(time);
        break;
    case 1:
        time->month = time->month == 0x01 ? 0x12 : bcd8_dec(time->month);
        rx8025_time_fix_weekday(time);
        break;
    case 2:
        if (time->day == 1)
            time->day = day_in_month_bcd(time->year, time->month);
        else
            time->day = bcd8_dec(time->day);
        rx8025_time_fix_weekday(time);
        break;
    case 3:
        time->hour = time->hour == 0x00 ? 0x23 : bcd8_dec(time->hour);
        break;
    case 4:
        time->minute = time->minute == 0x00 ? 0x59 : bcd8_dec(time->minute);
        break;
    case 5:
        time->second = time->second == 0x00 ? 0x59 : bcd8_dec(time->second);
        break;
    }
}
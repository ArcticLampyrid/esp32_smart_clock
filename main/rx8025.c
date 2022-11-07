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
void rx8025_fix_weekday(struct rx8025_time_t* time)
{
    int d = bcd8_to_uint8(time->day);
    int m = bcd8_to_uint8(time->month);
    int y = 2000 + bcd8_to_uint8(time->year);
    int weekday = (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7;
    time->weekday = 1 << weekday;
}
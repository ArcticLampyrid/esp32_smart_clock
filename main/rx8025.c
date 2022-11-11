#include "rx8025.h"
#include <driver/i2c.h>
#include "pin_layout.h"
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

static esp_err_t rx8025_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_NUM, RX8025_I2C_ADDR, &reg_addr, 1, data, len, pdMS_TO_TICKS(RX8025_TIMEOUT_MS));
}

static esp_err_t rx8025_register_write_byte(uint8_t reg_addr, uint8_t data)
{
    esp_err_t ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_NUM, RX8025_I2C_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(RX8025_TIMEOUT_MS));
    return ret;
}

void rx8025_init()
{
    /* Control register (Reg F)
    (BIT7) CSEL1: Compensation interval Select 1
    (BIT6) CSEL0: Compensation interval Select 0
    (BIT5) UIE  : Update Interrupt Enable
    (BIT4) TIE  : Timer Interrupt Enable
    (BIT3) AIE  : Alarm Interrupt Enable
    (BIT2) !    : <write-protected bits>
    (BIT1) !    : <write-protected bits>
    (BIT0) RESET: 0-Normal, 1-Stop
    */
    rx8025_register_write_byte(0x0F, BIT6);

    /* Flag register (Reg-E)
    (BIT7) !    : <write-protected bits>
    (BIT6) !    : <write-protected bits>
    (BIT5) UF   : Update Flag
    (BIT4) TF   : Timer Flag
    (BIT3) AF   : Alarm Flag
    (BIT2) !    : <write-protected bits>
    (BIT1) VLF  : Voltage Low Flag
    (BIT0) VDET : Voltage Detection Flag
    */
    rx8025_register_write_byte(0x0E, 0);

    /* Extension register (Reg-D)
    (BIT7) TEST : Manufacturer's test bit
    (BIT6) WADA : Week Alarm/Day Alarm
    (BIT5) USEL : Update Interrupt Select
    (BIT4) TE   : Timer Enable
    (BIT3) FSEL1: FOUT frequency Select 1
    (BIT2) FSEL0: FOUT frequency Select 0
    (BIT1) TSEL1: Timer Select 1
    (BIT0) TSEL0: Timer Select 0
    */
    rx8025_register_write_byte(0x0D, BIT1 | BIT6);

    rx8025_set_time(rx8025_time_min_value());
}
void rx8025_set_time(struct rx8025_time_t time)
{
    rx8025_register_write_byte(0x06, time.year);
    rx8025_register_write_byte(0x05, time.month);
    rx8025_register_write_byte(0x04, time.day);
    rx8025_register_write_byte(0x03, time.weekday);
    rx8025_register_write_byte(0x02, time.hour);
    rx8025_register_write_byte(0x01, time.minute);
    rx8025_register_write_byte(0x00, time.second);
}
struct rx8025_time_t rx8025_get_time()
{
    struct rx8025_time_t time;
    rx8025_register_read(0x06, &time.year, 1);
    rx8025_register_read(0x05, &time.month, 1);
    rx8025_register_read(0x04, &time.day, 1);
    rx8025_register_read(0x03, &time.weekday, 1);
    rx8025_register_read(0x02, &time.hour, 1);
    rx8025_register_read(0x01, &time.minute, 1);
    rx8025_register_read(0x00, &time.second, 1);
    return time;
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
struct rx8025_time_t rx8025_time_from_tm(struct tm *it)
{
    struct rx8025_time_t result = {
        .year = uint8_to_bcd8(it->tm_year - 100),
        .month = uint8_to_bcd8(it->tm_mon + 1),
        .day = uint8_to_bcd8(it->tm_mday),
        .hour = uint8_to_bcd8(it->tm_hour),
        .minute = uint8_to_bcd8(it->tm_min),
        .second = uint8_to_bcd8(it->tm_sec),
    };
    rx8025_time_fix_weekday(&result);
    return result;
}
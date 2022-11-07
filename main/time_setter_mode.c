#include "time_setter_mode.h"
#include "homepage_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
static char TAG[] = "clock_time_setter_mode";
struct time_setter_mode_t;
static void set_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before);
static void time_setter_on_refresh(struct time_setter_mode_t *mode);

struct time_setter_mode_t
{
    struct base_mode_info_t base;
    struct rx8025_time_t time;
    TickType_t cycle_begin;
    int progress;
    int display_cycle;
};
static struct time_setter_mode_t time_setter_mode = {
    .base = {.set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)time_setter_on_refresh}};
void switch_to_time_setter()
{
    time_setter_mode.time = rx8025_get_time();
    time_setter_mode.progress = 0;
    time_setter_mode.display_cycle = -1;
    time_setter_mode.cycle_begin = xTaskGetTickCount();
    g_currect_mode = (struct base_mode_info_t *)&time_setter_mode;
    ESP_LOGI(TAG, "switch to time setter");
}
static void set_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed, progress = %d", mode->progress);
    mode->progress++;
    mode->display_cycle = -1;
}
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
static void up_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "up_key_on_pressed, progress = %d", mode->progress);
    switch (mode->progress)
    {
    case 0:
        mode->time.year = mode->time.year == 0x99 ? 0x00 : bcd8_inc(mode->time.year);
        rx8025_fix_weekday(&mode->time);
        break;
    case 1:
        mode->time.month = mode->time.month == 0x12 ? 0x01 : bcd8_inc(mode->time.month);
        rx8025_fix_weekday(&mode->time);
        break;
    case 2:
        if (mode->time.day == day_in_month_bcd(mode->time.year, mode->time.month))
            mode->time.day = 0x01;
        else
            mode->time.day = bcd8_inc(mode->time.day);
        rx8025_fix_weekday(&mode->time);
        break;
    case 3:
        mode->time.hour = mode->time.hour == 0x23 ? 0x00 : bcd8_inc(mode->time.hour);
        break;
    case 4:
        mode->time.minute = mode->time.minute == 0x59 ? 0x00 : bcd8_inc(mode->time.minute);
        break;
    case 5:
        mode->time.second = mode->time.second == 0x59 ? 0x00 : bcd8_inc(mode->time.second);
        break;
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}
static void down_key_on_pressed(struct time_setter_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "down_key_on_pressed, progress = %d", mode->progress);
    switch (mode->progress)
    {
    case 0:
        mode->time.year = mode->time.year == 0x00 ? 0x99 : bcd8_dec(mode->time.year);
        rx8025_fix_weekday(&mode->time);
        break;
    case 1:
        mode->time.month = mode->time.month == 0x01 ? 0x12 : bcd8_dec(mode->time.month);
        rx8025_fix_weekday(&mode->time);
        break;
    case 2:
        if (mode->time.day == 1)
            mode->time.day = day_in_month_bcd(mode->time.year, mode->time.month);
        else
            mode->time.day = bcd8_dec(mode->time.day);
        rx8025_fix_weekday(&mode->time);
        break;
    case 3:
        mode->time.hour = mode->time.hour == 0x00 ? 0x23 : bcd8_dec(mode->time.hour);
        break;
    case 4:
        mode->time.minute = mode->time.minute == 0x00 ? 0x59 : bcd8_dec(mode->time.minute);
        break;
    case 5:
        mode->time.second = mode->time.second == 0x00 ? 0x59 : bcd8_dec(mode->time.second);
        break;
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}

static char *WEEKDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static void time_setter_on_refresh(struct time_setter_mode_t *mode)
{
    // 刷新是个非常耗时的工作，尽量减小刷新次数（并且可以提高按钮响应速度）
    int currect_display_cycle = ((xTaskGetTickCount() - mode->cycle_begin) % pdMS_TO_TICKS(360)) >= pdMS_TO_TICKS(180);
    if (mode->display_cycle == currect_display_cycle)
        return;
    mode->display_cycle = currect_display_cycle;

    char buf1[] = "20xx/xx/xx xxx";
    char buf2[] = "xx:xx:xx";
    struct rx8025_time_t time = mode->time;
    bcd8_to_dchar(&buf1[2], time.year);
    bcd8_to_dchar(&buf1[5], time.month);
    bcd8_to_dchar(&buf1[8], time.day);
    memcpy(&buf1[11], WEEKDAY_NAMES[__builtin_ctz(time.weekday)], 3);
    bcd8_to_dchar(&buf2[0], time.hour);
    bcd8_to_dchar(&buf2[3], time.minute);
    bcd8_to_dchar(&buf2[6], time.second);
    if (currect_display_cycle == 0)
    {
        switch (mode->progress)
        {
        case 0:
            memset(&buf1[0], '_', 4);
            break;
        case 1:
            memset(&buf1[5], '_', 2);
            break;
        case 2:
            memset(&buf1[8], '_', 2);
            break;
        case 3:
            memset(&buf2[0], '_', 2);
            break;
        case 4:
            memset(&buf2[3], '_', 2);
            break;
        case 5:
            memset(&buf2[6], '_', 2);
            break;
        default:
            rx8025_set_time(mode->time);
            switch_to_homepage();
            break;
        }
    }

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 0, 12, buf1);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB18_tr);
    u8g2_DrawStr(&u8g2, 0, 32, buf2);
    u8g2_SendBuffer(&u8g2);
}
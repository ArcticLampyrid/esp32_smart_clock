#include "homepage_mode.h"
#include "time_setter_mode.h"
#include "weather_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
static char TAG[] = "clock_homepage_mode";
struct homepage_mode_t;
static void mode_key_on_pressed(struct homepage_mode_t *mode, enum key_state_t before);
static void set_key_on_long_pressed(struct homepage_mode_t *mode, enum key_state_t before);
static void homepage_on_refresh(struct homepage_mode_t *mode);

struct homepage_mode_t
{
    struct base_mode_info_t base;
};
static struct homepage_mode_t homepage_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)mode_key_on_pressed},
             .set_key = {.on_long_pressed = (key_handler_t)set_key_on_long_pressed},
             .on_refresh = (on_refresh_t)homepage_on_refresh}};
void switch_to_homepage()
{
    g_currect_mode = (struct base_mode_info_t *)&homepage_mode;
    ESP_LOGI(TAG, "switch to homepage");
}
static void mode_key_on_pressed(struct homepage_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_weather();
}
static void set_key_on_long_pressed(struct homepage_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_long_pressed");
    switch_to_time_setter();
}
static char *WEEKDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static void homepage_on_refresh(struct homepage_mode_t *mode)
{
    char buf1[] = "20xx/xx/xx xxx";
    char buf2[] = "xx:xx:xx";
    struct rx8025_time_t time = rx8025_get_time();
    bcd8_to_dchar(&buf1[2], time.year);
    bcd8_to_dchar(&buf1[5], time.month);
    bcd8_to_dchar(&buf1[8], time.day);
    memcpy(&buf1[11], WEEKDAY_NAMES[__builtin_ctz(time.weekday)], 3);
    bcd8_to_dchar(&buf2[0], time.hour);
    bcd8_to_dchar(&buf2[3], time.minute);
    bcd8_to_dchar(&buf2[6], time.second);

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 0, 12, buf1);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB18_tr);
    u8g2_DrawStr(&u8g2, 0, 32, buf2);
    u8g2_SendBuffer(&u8g2);
}
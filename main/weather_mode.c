#include "weather_mode.h"
#include "alarm_listview_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include "weather.h"
static char TAG[] = "clock_weather_mode";
struct weather_mode_t;
static void mode_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before);
static void set_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before);
static void weather_on_refresh(struct weather_mode_t *mode);

struct weather_mode_t
{
    struct base_mode_info_t base;
    size_t index;
};
static struct weather_mode_t weather_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)mode_key_on_pressed},
             .set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)weather_on_refresh}};
void switch_to_weather()
{
    weather_mode.index = 0;
    g_currect_mode = (struct base_mode_info_t *)&weather_mode;
    ESP_LOGI(TAG, "switch to weather");
}
static void mode_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_alarm_listview(0);
}
static void up_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "weather of prev day");
    mode->index = g_weather_info.num_of_days == 0
                      ? 0
                      : (mode->index + g_weather_info.num_of_days - 1) % g_weather_info.num_of_days;
}
static void down_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "weather of next day");
    mode->index = g_weather_info.num_of_days == 0
                      ? 0
                      : (mode->index + 1) % g_weather_info.num_of_days;
}
static void set_key_on_pressed(struct weather_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed");
    weather_update(&g_weather_info);
}

static void weather_on_refresh(struct weather_mode_t *mode)
{
    char buf[64];
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_wqy12_t_gb2312);
    sprintf(buf, "City: %s", g_weather_info.city);
    u8g2_DrawUTF8(&u8g2, 0, 12, buf);
    const weather_of_day_t *weather_of_day = &g_weather_info.data[mode->index];
    sprintf(buf, "Day %02x-%02x: %s",
            weather_of_day->month,
            weather_of_day->day,
            weather_of_day->weather);
    u8g2_DrawUTF8(&u8g2, 0, 28, buf);
    sprintf(buf, "Tmp: %d℃~%d℃",
            weather_of_day->temperature_of_day,
            weather_of_day->temperature_of_night);
    u8g2_DrawUTF8(&u8g2, 0, 44, buf);
    sprintf(buf, "(20%02x-%02x-%02x %02x:%02x:%02x)",
            g_weather_info.updated_at.year,
            g_weather_info.updated_at.month,
            g_weather_info.updated_at.day,
            g_weather_info.updated_at.hour,
            g_weather_info.updated_at.minute,
            g_weather_info.updated_at.second);
    u8g2_DrawStr(&u8g2, 0, 60, buf);
    u8g2_SendBuffer(&u8g2);
}
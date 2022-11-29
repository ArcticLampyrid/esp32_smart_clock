#include "timer_mode.h"
#include "wifi_config_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <freertos/task.h>
#include <string.h>
#include "rx8025.h"
static char TAG[] = "clock_timer_mode";
struct timer_mode_t;
static void mode_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before);
static void set_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before);
static void timer_on_refresh(struct timer_mode_t *mode);

struct timer_mode_t
{
    struct base_mode_info_t base;
    TickType_t begin;
    TickType_t end;
    bool completed;
};
static struct timer_mode_t timer_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)mode_key_on_pressed},
             .set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)timer_on_refresh},
    .completed = true};
void switch_to_timer()
{
    g_currect_mode = (struct base_mode_info_t *)&timer_mode;
    ESP_LOGI(TAG, "switch to timer");
}
static void mode_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_wifi_config();
}
static void up_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "up_key_on_pressed");
    mode->completed ^= 1;
    if (!mode->completed)
    {
        mode->begin = xTaskGetTickCount() - mode->end + mode->begin;
    }
    else
    {
        mode->end = xTaskGetTickCount();
    }
}
static void down_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "down_key_on_pressed");
    mode->completed ^= 1;
    if (!mode->completed)
    {
        mode->begin = xTaskGetTickCount() - mode->end + mode->begin;
    }
    else
    {
        mode->end = xTaskGetTickCount();
    }
}
static void set_key_on_pressed(struct timer_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed");
    mode->begin = mode->end = xTaskGetTickCount();
}

static void timer_on_refresh(struct timer_mode_t *mode)
{
    TickType_t counter = (mode->completed ? mode->end : xTaskGetTickCount()) - mode->begin;
    TickType_t counter_in_ms = pdTICKS_TO_MS(counter);
    char buf[64];
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    sprintf(buf, "Timer: %02d''%02d'%d",
            counter_in_ms / 1000 / 60 % 60,
            counter_in_ms / 1000 % 60,
            counter_in_ms / 100 % 10);
    u8g2_DrawStr(&u8g2, 0, 12, buf);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 28, "Press set key to reset, ");
    u8g2_DrawStr(&u8g2, 0, 44, "up/down to toggle. ");
    u8g2_SendBuffer(&u8g2);
}
#include "alarm_listview_mode.h"
#include "alarm_controller_mode.h"
#include "time_setter_mode.h"
#include "wifi_config_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "rx8025.h"
#include "alarm.h"
static char TAG[] = "clock_alarm_listview_mode";
struct alarm_listview_mode_t;
static void mode_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before);
static void set_key_on_long_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before);
static void set_key_on_released(struct alarm_listview_mode_t *mode, enum key_state_t before);
static void alarm_listview_on_refresh(struct alarm_listview_mode_t *mode);
static void up_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before);

struct alarm_listview_mode_t
{
    struct base_mode_info_t base;
    int index;
    bool redraw;
};
static struct alarm_listview_mode_t alarm_listview_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)mode_key_on_pressed},
             .set_key = {
                 .on_long_pressed = (key_handler_t)set_key_on_long_pressed,
                 .on_released = (key_handler_t)set_key_on_released,
             },
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)alarm_listview_on_refresh}};
void switch_to_alarm_listview(int index)
{
    alarm_listview_mode.index = index;
    alarm_listview_mode.redraw = true;
    g_currect_mode = (struct base_mode_info_t *)&alarm_listview_mode;
    ESP_LOGI(TAG, "switch to alarm_listview");
}
static void mode_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_wifi_config();
}

static void set_key_on_long_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_long_pressed, currect index: %d", mode->index);
    if (mode->index < 0 || mode->index >= alarm_list.count)
    {
        switch_to_alarm_controller();
        return;
    }
    mode->index = alarm_list.count - 1;
    struct base_alarm_t *it = alarm_list.data[mode->index];
    it->switch_to_config(it, mode->index);
}
static void up_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before)
{
    mode->index--;
    if (mode->index < 0)
        mode->index = 0;
    alarm_listview_mode.redraw = true;
}
static void down_key_on_pressed(struct alarm_listview_mode_t *mode, enum key_state_t before)
{
    mode->index++;
    if (mode->index > alarm_list.count)
        mode->index = alarm_list.count;
    alarm_listview_mode.redraw = true;
}
static void set_key_on_released(struct alarm_listview_mode_t *mode, enum key_state_t before)
{
    if (before == KEY_STATE_PRESSED)
    {
        if (mode->index >= 0 && mode->index < alarm_list.count)
        {
            struct base_alarm_t *it = alarm_list.data[mode->index];
            it->enabled ^= 0x1;
            reschedule_alarm(&scheduled_alarm_info, &alarm_list);
            alarm_listview_mode.redraw = true;
        }
    }
}
static void alarm_listview_on_refresh(struct alarm_listview_mode_t *mode)
{
    if (!alarm_listview_mode.redraw)
    {
        vTaskDelay(0);
    }
    alarm_listview_mode.redraw = false;
    if (mode->index >= 0 && mode->index < alarm_list.count)
    {
        struct base_alarm_t *it = alarm_list.data[mode->index];
        it->display(it, mode->index);
    }
    else
    {
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, "Long press set key");
        u8g2_DrawStr(&u8g2, 0, 28, "to add a new alarm");
        u8g2_SendBuffer(&u8g2);
    }
}
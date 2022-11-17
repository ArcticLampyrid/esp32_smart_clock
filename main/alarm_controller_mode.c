#include "alarm_controller_mode.h"
#include "time_setter_mode.h"
#include "alarm_listview_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include "alarm.h"
#include "generic_alarm.h"
#include "hourly_chime.h"
#define OPTION_COUNT 3
static char TAG[] = "clock_alarm_controller_mode";
struct alarm_controller_mode_t;
static void set_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before);
static void alarm_controller_on_refresh(struct alarm_controller_mode_t *mode);

struct alarm_controller_mode_t
{
    struct base_mode_info_t base;
    int option_index;
};
static struct alarm_controller_mode_t alarm_controller_mode = {
    .base = {
        .set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
        .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
        .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
        .on_refresh = (on_refresh_t)alarm_controller_on_refresh}};

void switch_to_alarm_controller()
{
    alarm_controller_mode.option_index = 0;
    g_currect_mode = (struct base_mode_info_t *)&alarm_controller_mode;
    ESP_LOGI(TAG, "switch to alarm_controller");
}
static void set_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed");
    switch (mode->option_index)
    {
    case 0:
        arraylist_of_alarm_add(&alarm_list, generic_alarm_new());
        switch_to_alarm_listview(alarm_list.count - 1);
        break;
    case 1:
        arraylist_of_alarm_add(&alarm_list, hourly_chime_new());
        switch_to_alarm_listview(alarm_list.count - 1);
        break;
    default:
        switch_to_alarm_listview(alarm_list.count);
        break;
    }
}

static void up_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "up_key_on_pressed");
    mode->option_index = (mode->option_index + OPTION_COUNT - 1) % OPTION_COUNT;
    ESP_LOGI(TAG, "option[%d] selected", mode->option_index);
}

static void down_key_on_pressed(struct alarm_controller_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "down_key_on_pressed");
    mode->option_index = (mode->option_index + 1) % OPTION_COUNT;
    ESP_LOGI(TAG, "option[%d] selected", mode->option_index);
}

static void alarm_controller_on_refresh(struct alarm_controller_mode_t *mode)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
    u8g2_DrawStr(&u8g2, 20, 12, "Add alarm");
    u8g2_DrawStr(&u8g2, 20, 32, "Add hourly chime");
    u8g2_DrawStr(&u8g2, 20, 52, "Return");
    u8g2_DrawBox(&u8g2, 0, 20 * mode->option_index, 16, 16);
    u8g2_SendBuffer(&u8g2);
}
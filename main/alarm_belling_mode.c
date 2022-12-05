#include "homepage_mode.h"
#include "alarm_belling_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include "mp3.h"
static char TAG[] = "clock_alarm_belling_mode";
struct alarm_belling_mode_t;
static void any_key_on_pressed(struct alarm_belling_mode_t *mode, enum key_state_t before);
static void alarm_belling_on_refresh(struct alarm_belling_mode_t *mode);

struct alarm_belling_mode_t
{
    struct base_mode_info_t base;
};
static struct alarm_belling_mode_t alarm_belling_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)any_key_on_pressed},
             .set_key = {.on_pressed = (key_handler_t)any_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)any_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)any_key_on_pressed},
             .on_refresh = (on_refresh_t)alarm_belling_on_refresh}};
void switch_to_alarm_belling_mode()
{
    g_currect_mode = (struct base_mode_info_t *)&alarm_belling_mode;
    ESP_LOGI(TAG, "switch to alarm_belling");
}

bool is_alarm_belling_mode()
{
    return g_currect_mode == (struct base_mode_info_t *)&alarm_belling_mode;
}

static void any_key_on_pressed(struct alarm_belling_mode_t *mode, enum key_state_t before)
{
    mp3_stop();
    switch_to_homepage();
}

static void alarm_belling_on_refresh(struct alarm_belling_mode_t *mode)
{
    char buf1[] = "Alarm belling";
    char buf2[] = "xx:xx:xx";
    struct rx8025_time_t time = rx8025_get_time();
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
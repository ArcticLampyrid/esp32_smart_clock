#include "hourly_chime_config_mode.h"
#include "alarm_listview_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
static char TAG[] = "clock_hourly_chime_config_mode";
struct hourly_chime_config_mode_t;
static void set_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before);
static void hourly_chime_config_on_refresh(struct hourly_chime_config_mode_t *mode);

struct hourly_chime_config_mode_t
{
    struct base_mode_info_t base;
    struct hourly_chime_t *alarm;
    int index;
    TickType_t cycle_begin;
    int progress;
    int display_cycle;
};
static struct hourly_chime_config_mode_t hourly_chime_config_mode = {
    .base = {.set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)hourly_chime_config_on_refresh}};
void switch_to_hourly_chime_config(struct hourly_chime_t *alarm, int index)
{
    hourly_chime_config_mode.alarm = alarm;
    hourly_chime_config_mode.index = index;
    hourly_chime_config_mode.progress = 0;
    hourly_chime_config_mode.display_cycle = -1;
    hourly_chime_config_mode.cycle_begin = xTaskGetTickCount();
    g_currect_mode = (struct base_mode_info_t *)&hourly_chime_config_mode;
    ESP_LOGI(TAG, "switch to alarm config mode");
}
static void set_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed, progress = %d", mode->progress);
    mode->progress++;
    mode->display_cycle = -1;
    if (mode->progress == 24)
    {
        switch_to_alarm_listview(mode->index);
        reschedule_alarm(&scheduled_alarm_info, &alarm_list);
    }
}
static void up_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "up_key_on_pressed, progress = %d", mode->progress);
    if (mode->progress >= 0 && mode->progress < 24)
    {
        mode->alarm->hour_mask ^= (1 << mode->progress);
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}
static void down_key_on_pressed(struct hourly_chime_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "down_key_on_pressed, progress = %d", mode->progress);
    if (mode->progress >= 0 && mode->progress < 24)
    {
        mode->alarm->hour_mask ^= (1 << mode->progress);
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}

static void hourly_chime_config_on_refresh(struct hourly_chime_config_mode_t *mode)
{
    // 刷新是个非常耗时的工作，尽量减小刷新次数（并且可以提高按钮响应速度）
    int currect_display_cycle = ((xTaskGetTickCount() - mode->cycle_begin) % pdMS_TO_TICKS(360)) >= pdMS_TO_TICKS(180);
    if (mode->display_cycle == currect_display_cycle)
        return;
    mode->display_cycle = currect_display_cycle;
    char buf1[] = "Seq xx(EN): hourly";
    int seq = mode->index + 1;
    buf1[4] = (seq / 10 % 10) + '0';
    buf1[5] = (seq % 10) + '0';
    if (!mode->alarm->base.enabled)
    {
        buf1[7] = 'D';
        buf1[8] = 'D';
    }
    if (mode->progress < 12)
    {
        char buf2[] = " _00 _01 _02 _03 _04 _05";
        char buf3[] = " _06 _07 _08 _09 _10 _11";
        for (size_t i = 0; i < 6; i++)
        {
            buf2[1 + 4 * i] = (mode->alarm->hour_mask & (1 << i)) ? '*' : '_';
            if (currect_display_cycle == 0 && mode->progress == i)
            {
                buf2[1 + 4 * i] = ' ';
            }
        }
        for (size_t i = 0; i < 6; i++)
        {
            buf3[1 + 4 * i] = (mode->alarm->hour_mask & (1 << (i + 6))) ? '*' : '_';
            if (currect_display_cycle == 0 && mode->progress == i + 6)
            {
                buf3[1 + 4 * i] = ' ';
            }
        }
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, buf1);
        u8g2_DrawStr(&u8g2, 0, 32, buf2);
        u8g2_DrawStr(&u8g2, 0, 52, buf3);
        u8g2_SendBuffer(&u8g2);
    }
    else
    {
        char buf2[] = " _12 _13 _14 _15 _16 _17";
        char buf3[] = " _18 _19 _20 _21 _22 _23";
        for (size_t i = 0; i < 6; i++)
        {
            buf2[1 + 4 * i] = (mode->alarm->hour_mask & (1 << (i + 12))) ? '*' : '_';
            if (currect_display_cycle == 0 && mode->progress == i + 12)
            {
                buf2[1 + 4 * i] = ' ';
            }
        }
        for (size_t i = 0; i < 6; i++)
        {
            buf3[1 + 4 * i] = (mode->alarm->hour_mask & (1 << (i + 18))) ? '*' : '_';
            if (currect_display_cycle == 0 && mode->progress == i + 18)
            {
                buf3[1 + 4 * i] = ' ';
            }
        }
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, buf1);
        u8g2_DrawStr(&u8g2, 0, 32, buf2);
        u8g2_DrawStr(&u8g2, 0, 52, buf3);
        u8g2_SendBuffer(&u8g2);
    }
}
#include "generic_alarm_config_mode.h"
#include "alarm_listview_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <string.h>
#include "rx8025.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
static char TAG[] = "clock_generic_alarm_config_mode";
struct generic_alarm_config_mode_t;
static void set_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before);
static void up_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before);
static void down_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before);
static void generic_alarm_config_on_refresh(struct generic_alarm_config_mode_t *mode);

struct generic_alarm_config_mode_t
{
    struct base_mode_info_t base;
    struct generic_alarm_t *alarm;
    int index;
    TickType_t cycle_begin;
    int progress;
    int display_cycle;
};
static struct generic_alarm_config_mode_t generic_alarm_config_mode = {
    .base = {.set_key = {.on_pressed = (key_handler_t)set_key_on_pressed},
             .up_key = {.on_pressed = (key_handler_t)up_key_on_pressed},
             .down_key = {.on_pressed = (key_handler_t)down_key_on_pressed},
             .on_refresh = (on_refresh_t)generic_alarm_config_on_refresh}};
void switch_to_generic_alarm_config(struct generic_alarm_t *alarm, int index)
{
    generic_alarm_config_mode.alarm = alarm;
    generic_alarm_config_mode.index = index;
    generic_alarm_config_mode.progress = 0;
    generic_alarm_config_mode.display_cycle = -1;
    generic_alarm_config_mode.cycle_begin = xTaskGetTickCount();
    g_currect_mode = (struct base_mode_info_t *)&generic_alarm_config_mode;
    ESP_LOGI(TAG, "switch to alarm config mode");
}
static void set_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_pressed, progress = %d", mode->progress);
    mode->progress++;
    mode->display_cycle = -1;
    if (mode->progress == 11)
    {
        switch_to_alarm_listview(mode->index);
        reschedule_alarm(&scheduled_alarm_info, &alarm_list);
    }
}
static void up_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "up_key_on_pressed, progress = %d", mode->progress);
    switch (mode->progress)
    {
    case 0:
        mode->alarm->at_hour = mode->alarm->at_hour == 0x00 ? 0x23 : bcd8_dec(mode->alarm->at_hour);
        break;
    case 1:
        mode->alarm->at_minute = mode->alarm->at_minute == 0x00 ? 0x59 : bcd8_dec(mode->alarm->at_minute);
        break;
    case 2:
        mode->alarm->at_second = mode->alarm->at_second == 0x00 ? 0x59 : bcd8_dec(mode->alarm->at_second);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        mode->alarm->at_weekday ^= (1 << (mode->progress - 3));
        break;
    case 10:
        mode->alarm->bell_seq--;
        break;
    default:
        break;
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}
static void down_key_on_pressed(struct generic_alarm_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "down_key_on_pressed, progress = %d", mode->progress);
    switch (mode->progress)
    {
    case 0:
        mode->alarm->at_hour = mode->alarm->at_hour == 0x23 ? 0x00 : bcd8_inc(mode->alarm->at_hour);
        break;
    case 1:
        mode->alarm->at_minute = mode->alarm->at_minute == 0x59 ? 0x00 : bcd8_inc(mode->alarm->at_minute);
        break;
    case 2:
        mode->alarm->at_second = mode->alarm->at_second == 0x59 ? 0x00 : bcd8_inc(mode->alarm->at_second);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        mode->alarm->at_weekday ^= (1 << (mode->progress - 3));
        break;
    case 10:
        mode->alarm->bell_seq++;
        break;
    default:
        break;
    }
    mode->cycle_begin = xTaskGetTickCount();
    mode->display_cycle = -1;
}

static void generic_alarm_config_on_refresh(struct generic_alarm_config_mode_t *mode)
{
    // 刷新是个非常耗时的工作，尽量减小刷新次数（并且可以提高按钮响应速度）
    int currect_display_cycle = ((xTaskGetTickCount() - mode->cycle_begin) % pdMS_TO_TICKS(360)) >= pdMS_TO_TICKS(180);
    if (mode->display_cycle == currect_display_cycle)
        return;
    mode->display_cycle = currect_display_cycle;

    if (mode->progress <= 9)
    {
        char buf1[] = "Seq xx(EN): xx:xx:xx";
        char buf2[] = "Re:   Sun  Mon  Tue";
        char buf3[] = "  Wed  Thu  Fri  Sat";
        int seq = mode->index + 1;
        buf1[4] = (seq / 10 % 10) + '0';
        buf1[5] = (seq % 10) + '0';
        if (!mode->alarm->base.enabled)
        {
            buf1[7] = 'D';
            buf1[8] = 'D';
        }
        bcd8_to_dchar(&buf1[12], mode->alarm->at_hour);
        bcd8_to_dchar(&buf1[15], mode->alarm->at_minute);
        bcd8_to_dchar(&buf1[18], mode->alarm->at_second);

        buf2[5] = (mode->alarm->at_weekday & WEEKDAY_SUNDAY) ? '*' : '_';
        buf2[10] = (mode->alarm->at_weekday & WEEKDAY_MONDAY) ? '*' : '_';
        buf2[15] = (mode->alarm->at_weekday & WEEKDAY_TUESDAY) ? '*' : '_';
        buf3[1] = (mode->alarm->at_weekday & WEEKDAY_WEDNESDAY) ? '*' : '_';
        buf3[6] = (mode->alarm->at_weekday & WEEKDAY_THURSDAY) ? '*' : '_';
        buf3[11] = (mode->alarm->at_weekday & WEEKDAY_FRIDAY) ? '*' : '_';
        buf3[16] = (mode->alarm->at_weekday & WEEKDAY_SATURDAY) ? '*' : '_';

        if (currect_display_cycle == 0)
        {
            switch (mode->progress)
            {
            case 0:
                memset(&buf1[12], '_', 2);
                break;
            case 1:
                memset(&buf1[15], '_', 2);
                break;
            case 2:
                memset(&buf1[18], '_', 2);
                break;
            case 3:
                buf2[5] = ' ';
                break;
            case 4:
                buf2[10] = ' ';
                break;
            case 5:
                buf2[15] = ' ';
                break;
            case 6:
                buf3[1] = ' ';
                break;
            case 7:
                buf3[6] = ' ';
                break;
            case 8:
                buf3[11] = ' ';
                break;
            case 9:
                buf3[16] = ' ';
                break;
            default:
                break;
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

        char buf2[16];
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, "Bell Seq:");
        if (currect_display_cycle == 0)
        {
            u8g2_DrawStr(&u8g2, 0, 32, "___");
        }
        else
        {
            utoa(mode->alarm->bell_seq, buf2, 10);
            u8g2_DrawStr(&u8g2, 0, 32, buf2);
        }
        u8g2_SendBuffer(&u8g2);
    }
}
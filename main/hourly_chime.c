#include "hourly_chime.h"
#include "oled.h"
#include <esp_log.h>
#include "hourly_chime_config_mode.h"
#include "hourly_chime.h"
#include "mp3.h"
#include "alarm_belling_mode.h"
#include "weather.h"
#include "weather_speech.h"
#define MP3_FOLDER_HOURLY_ALARM 3
static char TAG[] = "hourly_chime";
static struct rx8025_time_t hourly_chime_schedule(struct hourly_chime_t *thiz, struct rx8025_time_t prev);
static void hourly_chime_play(struct hourly_chime_t *thiz);
static void hourly_chime_switch_to_config(struct hourly_chime_t *thiz, int index);
static void hourly_chime_display(struct hourly_chime_t *thiz, int index);

struct base_alarm_t *hourly_chime_new()
{
    struct hourly_chime_t *result = (struct hourly_chime_t *)malloc(sizeof(struct hourly_chime_t));
    result->base.enabled = false;
    result->base.schedule = (alarm_schedule_t)hourly_chime_schedule;
    result->base.play = (alarm_play_t)hourly_chime_play;
    result->base.switch_to_config = (alarm_switch_to_config_t)hourly_chime_switch_to_config;
    result->base.display = (alarm_switch_to_config_t)hourly_chime_display;
    result->hour_mask = 0x3FFF80; // bit[7:22]<-1 (excluding bit[22] but including bit[8])
    return (struct base_alarm_t *)result;
}
void hourly_chime_delete(struct base_alarm_t *dst)
{
    free(dst);
}
static struct rx8025_time_t hourly_chime_schedule(struct hourly_chime_t *thiz, struct rx8025_time_t prev)
{
    struct rx8025_time_t result = prev;
    result.minute = 0;
    result.second = 0;
    uint8_t hour_uint8 = bcd8_to_uint8(result.hour);
    do
    {
        if (hour_uint8 == 23)
        {
            hour_uint8 = 0;
            result = rx8025_time_next_day(result);
        }
        else
        {
            hour_uint8++;
        }
    } while ((thiz->hour_mask & (1 << hour_uint8)) == 0);
    result.hour = uint8_to_bcd8(hour_uint8);
    return result;
}
static void hourly_chime_play(struct hourly_chime_t *thiz)
{
    struct rx8025_time_t time = rx8025_get_time();
    ESP_LOGI(TAG, "hourly_chime_play");
    mp3_play_specified_folder(MP3_FOLDER_HOURLY_ALARM, (bcd8_to_uint8(time.hour) + 23) % 24 + 1);
}
static void hourly_chime_switch_to_config(struct hourly_chime_t *thiz, int index)
{
    ESP_LOGI(TAG, "hourly_chime_switch_to_config");
    switch_to_hourly_chime_config(thiz, index);
}
static void hourly_chime_display(struct hourly_chime_t *thiz, int index)
{
    char buf1[] = "Seq xx(EN): Hourly";
    char buf2[] = " xx hours specified";
    int seq = index + 1;
    buf1[4] = (seq / 10 % 10) + '0';
    buf1[5] = (seq % 10) + '0';
    if (!thiz->base.enabled)
    {
        buf1[7] = 'D';
        buf1[8] = 'D';
    }
    int hours_specified = __builtin_popcount(thiz->hour_mask);
    buf2[1] = (hours_specified / 10 % 10) + '0';
    buf2[2] = (hours_specified % 10) + '0';
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 0, 12, buf1);
    u8g2_DrawStr(&u8g2, 0, 32, buf2);
    u8g2_SendBuffer(&u8g2);
}
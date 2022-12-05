#include "generic_alarm.h"
#include "oled.h"
#include <esp_log.h>
#include "generic_alarm_config_mode.h"
#include "generic_alarm.h"
#include "mp3.h"
#include "alarm_belling_mode.h"
#include "weather.h"
#include "weather_speech.h"
#define MP3_FOLDER_GENERIC_ALARM 1
static char TAG[] = "generic_alarm";
static struct rx8025_time_t generic_alarm_schedule(struct generic_alarm_t *thiz, struct rx8025_time_t prev);
static void generic_alarm_play(struct generic_alarm_t *thiz);
static void generic_alarm_switch_to_config(struct generic_alarm_t *thiz, int index);
static void generic_alarm_display(struct generic_alarm_t *thiz, int index);

struct base_alarm_t *generic_alarm_new()
{
    struct generic_alarm_t *result = (struct generic_alarm_t *)malloc(sizeof(struct generic_alarm_t));
    result->base.enabled = false;
    result->base.schedule = (alarm_schedule_t)generic_alarm_schedule;
    result->base.play = (alarm_play_t)generic_alarm_play;
    result->base.switch_to_config = (alarm_switch_to_config_t)generic_alarm_switch_to_config;
    result->base.display = (alarm_switch_to_config_t)generic_alarm_display;
    result->base.delete_it = (alarm_delete_t)generic_alarm_delete;
    result->at_hour = result->at_minute = result->at_second = 0;
    result->at_weekday = 0x7F;
    result->bell_seq = 0;
    return (struct base_alarm_t *)result;
}
void generic_alarm_delete(struct base_alarm_t *dst)
{
    free(dst);
}
static struct rx8025_time_t generic_alarm_schedule(struct generic_alarm_t *thiz, struct rx8025_time_t prev)
{
    struct rx8025_time_t result = prev;
    result.hour = thiz->at_hour;
    result.minute = thiz->at_minute;
    result.second = thiz->at_second;
    if (rx8025_time_cmp(result, prev) <= 0)
    {
        result = rx8025_time_next_day(result);
    }
    if ((thiz->at_weekday & 0x7F) != 0)
    {
        while ((thiz->at_weekday & result.weekday) == 0)
        {
            result = rx8025_time_next_day(result);
        }
    }
    return result;
}
static void generic_alarm_play(struct generic_alarm_t *thiz)
{
    ESP_LOGI(TAG, "generic_alarm_play");
    if (thiz->bell_seq != 0)
    {
        // Normal alarm
        mp3_play_specified_folder(MP3_FOLDER_GENERIC_ALARM, thiz->bell_seq);
        switch_to_alarm_belling_mode();
    }
    else
    {
        // Weather reporter
        update_and_speak_weather_async();
    }
}
static void generic_alarm_switch_to_config(struct generic_alarm_t *thiz, int index)
{
    ESP_LOGI(TAG, "generic_alarm_switch_to_config");
    switch_to_generic_alarm_config(thiz, index);
}
static void generic_alarm_display(struct generic_alarm_t *thiz, int index)
{
    char buf1[] = "Seq xx(EN): xx:xx:xx";
    char buf2[] = "Re:   Sun  Mon  Tue";
    char buf3[] = "  Wed  Thu  Fri  Sat";
    int seq = index + 1;
    buf1[4] = (seq / 10 % 10) + '0';
    buf1[5] = (seq % 10) + '0';
    if (!thiz->base.enabled)
    {
        buf1[7] = 'D';
        buf1[8] = 'D';
    }
    bcd8_to_dchar(&buf1[12], thiz->at_hour);
    bcd8_to_dchar(&buf1[15], thiz->at_minute);
    bcd8_to_dchar(&buf1[18], thiz->at_second);

    buf2[5] = (thiz->at_weekday & WEEKDAY_SUNDAY) ? '*' : '_';
    buf2[10] = (thiz->at_weekday & WEEKDAY_MONDAY) ? '*' : '_';
    buf2[15] = (thiz->at_weekday & WEEKDAY_TUESDAY) ? '*' : '_';
    buf3[1] = (thiz->at_weekday & WEEKDAY_WEDNESDAY) ? '*' : '_';
    buf3[6] = (thiz->at_weekday & WEEKDAY_THURSDAY) ? '*' : '_';
    buf3[11] = (thiz->at_weekday & WEEKDAY_FRIDAY) ? '*' : '_';
    buf3[16] = (thiz->at_weekday & WEEKDAY_SATURDAY) ? '*' : '_';

    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
    u8g2_DrawStr(&u8g2, 0, 12, buf1);
    u8g2_DrawStr(&u8g2, 0, 32, buf2);
    u8g2_DrawStr(&u8g2, 0, 52, buf3);
    u8g2_SendBuffer(&u8g2);
}
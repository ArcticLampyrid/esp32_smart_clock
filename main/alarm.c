#include "alarm.h"
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
static char TAG[] = "clock_alarm";
struct scheduled_alarm_info_t scheduled_alarm_info;
arraylist_of_alarm_t alarm_list;
arraylist_of_alarm_t arraylist_of_alarm_new()
{
    arraylist_of_alarm_t result = {
        .data = (struct base_alarm_t **)malloc(sizeof(intptr_t) * 4),
        .count = 0,
        .capacity = 4,
    };
    return result;
}
void arraylist_of_alarm_add(arraylist_of_alarm_t *thiz, struct base_alarm_t *member)
{
    if (thiz->count == thiz->capacity)
    {
        if (thiz->capacity == 0)
        {
            thiz->capacity = 4;
            thiz->data = (struct base_alarm_t **)malloc(sizeof(intptr_t) * 4);
        }
        else
        {
            size_t new_capacity = thiz->capacity + thiz->capacity / 2;
            struct base_alarm_t **new_buf = (struct base_alarm_t **)malloc(sizeof(intptr_t) * new_capacity);
            memcpy(new_buf, thiz->data, sizeof(intptr_t) * thiz->count);
            free(thiz->data);
            thiz->data = new_buf;
            thiz->capacity = new_capacity;
        }
    }
    thiz->data[thiz->count++] = member;
}
void arraylist_of_alarm_delete(arraylist_of_alarm_t *thiz)
{
    free(thiz->data);
    thiz->data = NULL;
    thiz->count = thiz->capacity = 0;
}
void arraylist_of_alarm_remove(arraylist_of_alarm_t *thiz, size_t index)
{
    memmove(thiz->data + index, thiz->data + index + 1, (thiz->count - index - 1) * sizeof(arraylist_of_alarm_t *));
    thiz->count--;
}
void reschedule_alarm(struct scheduled_alarm_info_t *dst, arraylist_of_alarm_t *arr)
{
    struct rx8025_time_t time = rx8025_get_time();
    struct rx8025_time_t scheduled_time = rx8025_time_max_value();
    struct base_alarm_t *scheduled_alarm = NULL;
    for (size_t i = 0; i < arr->count; i++)
    {
        struct base_alarm_t *alarm = arr->data[i];
        if (!alarm->enabled)
            continue;
        struct rx8025_time_t scheduled_time_for_currect = alarm->schedule(alarm, time);
        if (rx8025_time_cmp(scheduled_time, scheduled_time_for_currect) > 0)
        {
            scheduled_time = scheduled_time_for_currect;
            scheduled_alarm = alarm;
        }
    }
    dst->alarm = scheduled_alarm;
    dst->at = scheduled_time;
    if (scheduled_alarm == NULL)
    {
        ESP_LOGI(TAG, "no alarm scheduled");
    }
    else
    {
        ESP_LOGI(TAG, "alarm scheduled at 20%02x/%02x/%02x %02x:%02x:%02x",
                 scheduled_time.year, scheduled_time.month, scheduled_time.day,
                 scheduled_time.hour, scheduled_time.minute, scheduled_time.second);
    }
}
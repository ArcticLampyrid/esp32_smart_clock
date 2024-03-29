#pragma once
#include "rx8025.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
struct base_alarm_t;
typedef struct rx8025_time_t (*alarm_schedule_t)(struct base_alarm_t *thiz, struct rx8025_time_t prev);
typedef void (*alarm_play_t)(struct base_alarm_t *thiz);
typedef void (*alarm_switch_to_config_t)(struct base_alarm_t *thiz, int index);
typedef void (*alarm_display_t)(struct base_alarm_t *thiz, int index);
typedef void (*alarm_delete_t)(struct base_alarm_t *thiz);
struct base_alarm_t
{
    bool enabled;
    alarm_schedule_t schedule;
    alarm_play_t play;
    alarm_switch_to_config_t switch_to_config;
    alarm_display_t display;
    alarm_delete_t delete_it;
};
typedef struct
{
    struct base_alarm_t **data;
    size_t count;
    size_t capacity;
} arraylist_of_alarm_t;
arraylist_of_alarm_t *arraylist_of_alarm_new();
void arraylist_of_alarm_add(arraylist_of_alarm_t *thiz, struct base_alarm_t *member);
void arraylist_of_alarm_delete(arraylist_of_alarm_t *thiz);
void arraylist_of_alarm_clear(arraylist_of_alarm_t *thiz);
void arraylist_of_alarm_remove(arraylist_of_alarm_t *thiz, size_t index);
struct scheduled_alarm_info_t
{
    arraylist_of_alarm_t *alarms;
    struct rx8025_time_t at;
};
void reschedule_alarm(struct scheduled_alarm_info_t *dst, arraylist_of_alarm_t *arr);
void alarm_init();

extern volatile struct scheduled_alarm_info_t scheduled_alarm_info;
extern arraylist_of_alarm_t *volatile alarm_list;

#pragma once
#include "alarm.h"
#include "bcd.h"
#include <stdint.h>
struct base_alarm_t *generic_alarm_new();
void generic_alarm_delete(struct base_alarm_t *);
struct generic_alarm_t
{
    struct base_alarm_t base;
    bcd8_t at_hour;
    bcd8_t at_minute;
    bcd8_t at_second;
    uint8_t at_weekday;
    uint8_t bell_seq;
};
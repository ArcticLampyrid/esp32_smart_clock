#pragma once
#include "alarm.h"
#include "bcd.h"
#include <stdint.h>
struct base_alarm_t *hourly_chime_new();
void hourly_chime_delete(struct base_alarm_t *);
struct hourly_chime_t
{
    struct base_alarm_t base;
    uint32_t hour_mask;
};
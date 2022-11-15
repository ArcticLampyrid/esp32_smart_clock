#pragma once
#include "rx8025.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#define MAX_WEATHER_DATA_COUNT 7
typedef struct weather_of_day_t
{
    bcd8_t year;
    bcd8_t month;
    bcd8_t day;
    char weather[32];
    int8_t temperature_of_day;
    int8_t temperature_of_night;
} weather_of_day_t;
typedef struct weather_info_t
{
    char city[32];
    struct rx8025_time_t updated_at;
    size_t num_of_days;
    weather_of_day_t data[MAX_WEATHER_DATA_COUNT];
} weather_info_t;
extern weather_info_t g_weather_info;
void weather_update(weather_info_t *thiz);
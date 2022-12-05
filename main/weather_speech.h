#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "weather.h"
void speak_weather_text(const char *text);
void speak_weather_temperature(int8_t temperature);
void speak_weather_full(const weather_of_day_t *weather_of_day);
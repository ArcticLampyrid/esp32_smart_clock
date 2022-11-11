#pragma once
#include <stdint.h>
// TODO: add more
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static char* TIME_ZONE_NAMES[] = {"CST-8", "EST+5", "EDT+4", "PST+8", "PDT+7"};
#pragma GCC diagnostic pop
void timezone_init();
void timezone_set(uint8_t id);
uint8_t timezone_get();
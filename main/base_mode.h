#pragma once
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
enum key_state_t
{
    KEY_STATE_RELEASED,
    KEY_STATE_PRESSED,
    KEY_STATE_LONG_PRESSED
};
struct base_mode_info_t;
typedef void (*key_handler_t)(struct base_mode_info_t *mode, enum key_state_t before);
typedef void (*on_refresh_t)(struct base_mode_info_t *mode);
struct key_handler_group_t
{
    key_handler_t on_pressed;
    key_handler_t on_released;
    key_handler_t on_long_pressed;
};
struct base_mode_info_t
{
    struct key_handler_group_t mode_key;
    struct key_handler_group_t set_key;
    struct key_handler_group_t up_key;
    struct key_handler_group_t down_key;
    on_refresh_t on_refresh;
};
extern struct base_mode_info_t *g_currect_mode;
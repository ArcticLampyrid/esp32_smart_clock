#include "key_dispatcher.h"
#include "pin_layout.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
static char TAG[] = "clock_key_dispatcher";
struct key_state_info_t
{
    gpio_num_t gpio_num;
    enum key_state_t state;
    TickType_t at_last_changed;
    TickType_t at_last_pressed;
    intptr_t handler_group_offset;
    struct base_mode_info_t *mode_at_last_pressed;
};
#define KEY_COUNT 4
static struct key_state_info_t state_info_for_keys[] = {
    {.gpio_num = GPIO_MODE_KEY,
     .state = KEY_STATE_RELEASED,
     .handler_group_offset = offsetof(struct base_mode_info_t, mode_key),
     .at_last_changed = 0,
     .at_last_pressed = 0,
     .mode_at_last_pressed = NULL},
    {.gpio_num = GPIO_SET_KEY,
     .state = KEY_STATE_RELEASED,
     .handler_group_offset = offsetof(struct base_mode_info_t, set_key),
     .at_last_changed = 0,
     .at_last_pressed = 0,
     .mode_at_last_pressed = NULL},
    {.gpio_num = GPIO_UP_KEY,
     .state = KEY_STATE_RELEASED,
     .handler_group_offset = offsetof(struct base_mode_info_t, up_key),
     .at_last_changed = 0,
     .at_last_pressed = 0,
     .mode_at_last_pressed = NULL},
    {.gpio_num = GPIO_DOWN_KEY,
     .state = KEY_STATE_RELEASED,
     .handler_group_offset = offsetof(struct base_mode_info_t, down_key),
     .at_last_changed = 0,
     .at_last_pressed = 0,
     .mode_at_last_pressed = NULL}};
inline struct key_handler_group_t *get_handler_group(struct base_mode_info_t *mode, struct key_state_info_t *state_info)
{
    return (struct key_handler_group_t *)((intptr_t)mode + state_info->handler_group_offset);
}
void dispatch_for_keys()
{
    for (int key_id = 0; key_id < KEY_COUNT; key_id++)
    {
        struct key_state_info_t *key = &state_info_for_keys[key_id];
        if (xTaskGetTickCount() - key->at_last_changed < pdMS_TO_TICKS(50))
        {
            continue;
        }
        bool gpio_state = gpio_get_level(key->gpio_num);
        if (key->state == KEY_STATE_RELEASED && gpio_state == 0)
        {
            key->state = KEY_STATE_PRESSED;
            key->mode_at_last_pressed = g_currect_mode;
            key->at_last_pressed = xTaskGetTickCount();
            key->at_last_changed = xTaskGetTickCount();
            struct key_handler_group_t *handler = get_handler_group(g_currect_mode, key);
            if (handler->on_pressed)
                handler->on_pressed(g_currect_mode, KEY_STATE_RELEASED);
        }
        else if (key->state != KEY_STATE_RELEASED && gpio_state == 1)
        {
            enum key_state_t before = key->state;
            struct base_mode_info_t *mode = key->mode_at_last_pressed;
            struct key_handler_group_t *handler = get_handler_group(mode, key);
            key->state = KEY_STATE_RELEASED;
            key->mode_at_last_pressed = NULL;
            key->at_last_changed = xTaskGetTickCount();
            if (handler->on_released)
                handler->on_released(mode, before);
        }
        else if (key->state == KEY_STATE_PRESSED && gpio_state == 0)
        {
            if (xTaskGetTickCount() - key->at_last_pressed >= pdMS_TO_TICKS(3000))
            {
                struct base_mode_info_t *mode = key->mode_at_last_pressed;
                struct key_handler_group_t *handler = get_handler_group(mode, key);
                key->state = KEY_STATE_LONG_PRESSED;
                key->at_last_changed = xTaskGetTickCount();
                if (handler->on_long_pressed)
                    handler->on_long_pressed(mode, KEY_STATE_PRESSED);
            }
        }
    }
}
#include "base_mode.h"
#include "wifi_config_mode.h"
#include "ntp_sync_config_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <string.h>
#include "rx8025.h"
static char TAG[] = "clock_wifi_config_mode";
struct wifi_config_mode_t;
static void mode_key_on_pressed(struct wifi_config_mode_t *mode, enum key_state_t before);
static void set_key_on_long_pressed(struct wifi_config_mode_t *mode, enum key_state_t before);
static void wifi_config_on_refresh(struct wifi_config_mode_t *mode);
struct wifi_config_mode_t
{
    struct base_mode_info_t base;
};
static struct wifi_config_mode_t wifi_config_mode = {
    .base = {.mode_key = {.on_pressed = (key_handler_t)mode_key_on_pressed},
             .set_key = {.on_long_pressed = (key_handler_t)set_key_on_long_pressed},
             .on_refresh = (on_refresh_t)wifi_config_on_refresh}};
void switch_to_wifi_config()
{
    g_currect_mode = (struct base_mode_info_t *)&wifi_config_mode;
    ESP_LOGI(TAG, "switch to wifi_config");
}
static void mode_key_on_pressed(struct wifi_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_ntp_sync_config();
}
static void set_key_on_long_pressed(struct wifi_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_long_pressed");
    wifi_reset();
}
static void wifi_config_on_refresh(struct wifi_config_mode_t *mode)
{
    wifi_mode_t wifi_mode;
    if (esp_wifi_get_mode(&wifi_mode) != ESP_OK)
    {
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_ncenB10_tr);
        u8g2_DrawStr(&u8g2, 0, 12, "WiFi Disabled");
        u8g2_SendBuffer(&u8g2);
        return;
    }
    switch (wifi_mode)
    {
    case WIFI_MODE_STA:
    {
        wifi_config_t wifi_config;
        esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
        char line1[128] = "SSID: ";
        strcat(line1, (char *)wifi_config.sta.ssid);
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, line1);
        if (wifi_state.smart_config_started)
        {
            u8g2_DrawStr(&u8g2, 0, 28, "Configuring...");
            u8g2_DrawStr(&u8g2, 0, 44, "via EspTouch/Airkiss");
        }
        else
        {
            u8g2_DrawStr(&u8g2, 0, 28, "Long press set key to");
            u8g2_DrawStr(&u8g2, 0, 44, "enter configuration mode");
        }
        u8g2_SendBuffer(&u8g2);
        break;
    }
    default:
        break;
    }
}
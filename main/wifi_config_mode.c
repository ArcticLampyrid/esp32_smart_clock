#include "wifi_config_mode.h"
#include "homepage_mode.h"
#include "wifi_config_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
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

void wifi_soft_ap_reset()
{
    srand(time(NULL));
    static char rand_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k'};
    esp_wifi_stop();
    char ssid[] = "SMARTCLK-001";
    char password[] = "CLKPWD-001";
    ssid[9] = rand_chars[rand() % sizeof(rand_chars)];
    ssid[10] = rand_chars[rand() % sizeof(rand_chars)];
    ssid[11] = rand_chars[rand() % sizeof(rand_chars)];
    password[7] = rand_chars[rand() % sizeof(rand_chars)];
    password[8] = rand_chars[rand() % sizeof(rand_chars)];
    password[9] = rand_chars[rand() % sizeof(rand_chars)];
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = 12,
            .channel = 1,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    strcpy((char*)wifi_config.ap.ssid, ssid);
    strcpy((char*)wifi_config.ap.password, password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}
static void mode_key_on_pressed(struct wifi_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "mode_key_on_pressed");
    switch_to_homepage();
}
static void set_key_on_long_pressed(struct wifi_config_mode_t *mode, enum key_state_t before)
{
    ESP_LOGI(TAG, "set_key_on_long_pressed");
    wifi_soft_ap_reset();
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
    case WIFI_MODE_AP:
    {
        wifi_config_t wifi_config;
        esp_wifi_get_config(WIFI_IF_AP, &wifi_config);
        char line1[128] = "SSID: ";
        char line2[128] = "Pwd: ";
        strcat(line1, (char *)wifi_config.ap.ssid);
        strcat(line2, (char *)wifi_config.ap.password);
        char line3[] = "(AP Mode)";
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_helvB08_tr);
        u8g2_DrawStr(&u8g2, 0, 12, line1);
        u8g2_DrawStr(&u8g2, 0, 28, line2);
        u8g2_DrawStr(&u8g2, 0, 44, line3);
        u8g2_SendBuffer(&u8g2);
        break;
    }
    default:
        break;
    }
}
#include "wifi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_wpa2.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_smartconfig.h>
#include <string.h>
#include <nvs_flash.h>
#include <nvs.h>
struct wifi_state_t wifi_state = {
    .smart_config_started = false,
};
static char TAG[] = "clock_wifi";

static const int ESPTOUCH_DONE_BIT = BIT1;
static EventGroupHandle_t s_wifi_event_group;
static void event_handler(void *arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void *event_data)
{
    if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG, "ssid and password fetched from smart config");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        char ssid[33] = {0};
        char password[65] = {0};
        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        wifi_connect(ssid, password);
        wifi_state.smart_config_started = false;
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void smartconfig_task(void *parm)
{
    esp_smartconfig_stop();
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    for (;;)
    {
        EventBits_t uxBits = xEventGroupWaitBits(s_wifi_event_group, ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smart config completed");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

void wifi_init()
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, SC_EVENT_GOT_SSID_PSWD, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_reset()
{
    esp_wifi_disconnect();
    wifi_state.smart_config_started = true;
    xEventGroupClearBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
}

void wifi_connect(char *ssid, char *password)
{
    esp_wifi_disconnect();
    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void wifi_connect_by_memory()
{
    wifi_config_t wifi_config;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (strlen((const char *)wifi_config.sta.ssid) != 0)
    {
        esp_wifi_connect();
    }
}
#include "ntp.h"
#include "sntp.h"
#include <time.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs.h>
#include "rx8025.h"
#include "alarm.h"
static char TAG[] = "clock_ntp";
static char STORAGE_NAMESPACE[] = "clock";
static void on_time_sync(struct timeval *tv)
{
    if (!ntp_sync_is_enabled())
    {
        ESP_LOGI(TAG, "ntp skipped due to configuration");
        return;
    }
    ESP_LOGI(TAG, "ntp synced");
    struct tm *nowtm;
    nowtm = localtime(&tv->tv_sec);
    rx8025_set_time(rx8025_time_from_tm(nowtm));
    reschedule_alarm(&scheduled_alarm_info, &alarm_list);
}
void ntp_sync_init()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_time_sync_notification_cb(on_time_sync);
    sntp_init();
}
bool ntp_sync_is_enabled()
{
    nvs_handle_t nvs_handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK)
    {
        uint8_t r;
        if (nvs_get_u8(nvs_handle, "ntp_sync_enabled", &r) == ESP_OK)
        {
            return r != 0;
        }
    }
    return true;
}
void ntp_sync_set_enabled(bool enabled)
{
    nvs_handle_t nvs_handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK)
    {
        nvs_set_u8(nvs_handle, "ntp_sync_enabled", (uint8_t)enabled);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "ntp_sync_enabled = %u", (uint8_t)enabled);
    }
}
void ntp_request_sync()
{
    sntp_restart();
}
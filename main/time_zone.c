#include "time_zone.h"
#include <nvs.h>
#include <time.h>
#include <esp_log.h>
static char STORAGE_NAMESPACE[] = "clock";
static char TAG[] = "timezone";
void timezone_init()
{
    uint8_t id = timezone_get();
    setenv("TZ", TIME_ZONE_NAMES[id], 1);
    tzset();
}
void timezone_set(uint8_t id)
{
    setenv("TZ", TIME_ZONE_NAMES[id], 1);
    tzset();

    nvs_handle_t nvs_handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK)
    {
        nvs_set_u8(nvs_handle, "timezone_id", id);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "timezone info saved, id: %u", id);
    }
}
uint8_t timezone_get()
{
    nvs_handle_t nvs_handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle) == ESP_OK)
    {
        uint8_t id;
        if (nvs_get_u8(nvs_handle, "timezone_id", &id) == ESP_OK)
        {
            return id;
        }
    }
    return 0;
}
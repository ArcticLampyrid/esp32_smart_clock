#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include "sdkconfig.h"
#include "base_mode.h"
#include "homepage_mode.h"
#include "wifi_config_mode.h"
#include "pin_layout.h"
#include "oled.h"
#include "key_dispatcher.h"
#include "rx8025.h"
static char TAG[] = "clock";
void task_dispatch_for_keys(void *pvParameters)
{
    for (;;)
    {
        dispatch_for_keys();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ull << GPIO_MODE_KEY) + (1ull << GPIO_SET_KEY) + (1ull << GPIO_UP_KEY) + (1ull << GPIO_DOWN_KEY);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    wifi_init();
    wifi_soft_ap_reset();

    rx8025_init();

    oled_init();

    oled_welcome();
    vTaskDelay(pdMS_TO_TICKS(2000));

    switch_to_homepage();
    xTaskCreate(task_dispatch_for_keys,
                "task_dispatch_for_keys",
                2000,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
    for (;;)
    {
        if (g_currect_mode->on_refresh)
            g_currect_mode->on_refresh(g_currect_mode);
    }

    // ESP_LOGI(TAG, "All done!");
}
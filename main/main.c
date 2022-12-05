#include "oled.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c.h>
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
#include "key_dispatcher.h"
#include "rx8025.h"
#include "alarm.h"
#include "mp3.h"
#include "time_zone.h"
#include "ntp.h"
#include "buzzer.h"
static char TAG[] = "clock";
void task_dispatch_for_keys(void *pvParameters)
{
    for (;;)
    {
        dispatch_for_keys();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void task_dispatch_for_alarms(void *pvParameters)
{
    for (;;)
    {
        if (scheduled_alarm_info.alarms->count != 0)
        {
            if (rx8025_time_cmp(scheduled_alarm_info.at, rx8025_get_time()) <= 0)
            {
                for (size_t i = 0; i < scheduled_alarm_info.alarms->count; i++)
                {
                    struct base_alarm_t *it = scheduled_alarm_info.alarms->data[i];
                    it->play(it);
                }
                reschedule_alarm(&scheduled_alarm_info, alarm_list);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main()
{
    buzzer_init();
    timezone_init();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ull << GPIO_MODE_KEY) | (1ull << GPIO_SET_KEY) | (1ull << GPIO_UP_KEY) | (1ull << GPIO_DOWN_KEY);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, // use external pullup resistor
        .scl_pullup_en = GPIO_PULLUP_DISABLE, // use external pullup resistor
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM, i2c_conf.mode, 0, 0, 0));

    alarm_init();

    mp3_init();
    mp3_reset();

    wifi_init();
    wifi_connect_by_memory();

    rx8025_init();

    oled_init();

    ESP_LOGI(TAG, "welcome to use SMARTCLK");
    oled_welcome();
    vTaskDelay(pdMS_TO_TICKS(2000));
    mp3_reset();

    switch_to_homepage();
    xTaskCreate(task_dispatch_for_keys,
                "dispatch_for_keys",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
    xTaskCreate(task_dispatch_for_alarms,
                "dispatch_for_alarms",
                8192,
                NULL,
                tskIDLE_PRIORITY,
                NULL);
    ntp_sync_init();

    for (;;)
    {
        if (g_currect_mode->on_refresh)
            g_currect_mode->on_refresh(g_currect_mode);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    // ESP_LOGI(TAG, "All done!");
}

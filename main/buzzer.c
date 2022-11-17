#include "buzzer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/ledc.h>
#include <esp_err.h>
#include "pin_layout.h"
static EventGroupHandle_t s_buzzer_event_group;
static const int EVENT_BEEP_BIT = BIT0;
static void buzzer_event_loop(void *parm)
{
    TickType_t ticksToWait = portMAX_DELAY;
    for (;;)
    {
        EventBits_t uxBits = xEventGroupWaitBits(
            s_buzzer_event_group,
            EVENT_BEEP_BIT,
            true,
            false,
            ticksToWait);
        ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0));
        if (uxBits & EVENT_BEEP_BIT)
        {
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL));
            ticksToWait = pdMS_TO_TICKS(200);
        }
        else
        {
            ticksToWait = portMAX_DELAY;
        }
    }
}
void buzzer_init()
{
    s_buzzer_event_group = xEventGroupCreate();
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BUZZER_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 4000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .timer_sel = BUZZER_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = IO_BUZZER,
        .duty = 4095,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0));
    xTaskCreate(buzzer_event_loop, "buzzer_event_loop", 2048, NULL, 3, NULL);
}
void buzzer_beep()
{
    xEventGroupSetBits(s_buzzer_event_group, EVENT_BEEP_BIT);
}
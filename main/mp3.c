#include "mp3.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "pin_layout.h"
#include <stddef.h>
static volatile struct
{
    uint32_t received_cmd_count;
    uint16_t number_of_files_on_tf;
    uint16_t volume;
    EventGroupHandle_t event_group;
} mp3_state;
static const int EVENT_PLAY_COMPLETED_BIT = BIT0;
static char TAG[] = "MP3";
static void mp3_receive_task(void *pvParameters)
{
    uint8_t buffer[10];
    uint8_t buffer_size = 10;
    for (;;)
    {
        int ret = uart_read_bytes(
            MP3_UART_NUM,
            &buffer[10 - buffer_size],
            buffer_size,
            portMAX_DELAY);
        if (ret < 0)
        {
            ESP_LOGE(TAG, "failed to receive data");
            continue;
        }
        if (ret == 0)
        {
            continue;
        }
        buffer_size -= (uint8_t)ret;
        if (buffer_size > 0)
        {
            ESP_LOGI(TAG, "partly received");
            continue;
        }
        buffer_size = 10;

        uint16_t sum = 0;
        for (int i = 1; i < 7; i++)
        {
            sum += buffer[i];
        }
        sum = -sum;
        uint16_t sum_received = buffer[7] << 8 | buffer[8];
        if (sum_received != sum)
        {
            ESP_LOGE(TAG, "chesksums of receive data mismatch");
            continue;
        }

        uint8_t command = buffer[3];
        uint16_t argument = buffer[5] << 8 | buffer[6];
        ESP_LOGI(TAG, "recv, command: 0x%x, argument: 0x%x", command, argument);

        switch (command)
        {
        case 0x3C:
        case 0x3D:
        case 0x3E:
            xEventGroupSetBits(mp3_state.event_group, EVENT_PLAY_COMPLETED_BIT);
            break;
        case 0x43:
            mp3_state.volume = argument;
            break;
        case 0x48:
            mp3_state.number_of_files_on_tf = argument;
            break;

        default:
            break;
        }

        // inc the count after refreshing the state
        mp3_state.received_cmd_count++;
    }
}

void mp3_init()
{
    mp3_state.event_group = xEventGroupCreate();
    mp3_state.received_cmd_count = 0;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(MP3_UART_NUM, 1024 * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MP3_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(MP3_UART_NUM, MP3_TXD, MP3_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    xTaskCreate(mp3_receive_task,
                "mp3_receive_task",
                2000,
                NULL,
                tskIDLE_PRIORITY + 3,
                NULL);
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ull << MP3_BUSY);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

static void mp3_cmd_no_wait(uint8_t command, uint16_t argument, bool ack)
{
    uint8_t buffer[] = {0x7E, 0xFF, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xEF};
    buffer[3] = command;
    buffer[4] = ack ? 1 : 0;
    buffer[5] = (uint8_t)(argument >> 8);
    buffer[6] = (uint8_t)(argument & 0xFF);
    uint16_t sum = 0;
    for (int i = 1; i < 7; i++)
    {
        sum += buffer[i];
    }
    sum = -sum;
    buffer[7] = (uint8_t)(sum >> 8);
    buffer[8] = (uint8_t)(sum & 0xFF);
    uart_write_bytes(MP3_UART_NUM, buffer, sizeof(buffer));
    ESP_ERROR_CHECK(uart_wait_tx_done(MP3_UART_NUM, portMAX_DELAY));
    ESP_LOGI(TAG, "send, command: 0x%x, argument: 0x%x", command, argument);
}
static bool mp3_cmd(uint8_t command, uint16_t argument, bool ack)
{
    TickType_t begin = xTaskGetTickCount();
    uint32_t cmd_count = mp3_state.received_cmd_count;
    mp3_cmd_no_wait(command, argument, ack);
    while (mp3_state.received_cmd_count == cmd_count)
    {
        if (xTaskGetTickCount() - begin > pdMS_TO_TICKS(1000))
        {
            ESP_LOGW(TAG, "mp3_cmd timeout");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return true;
}
uint16_t mp3_get_volume()
{
    mp3_cmd(0x43, 0, false);
    return mp3_state.volume;
}
uint16_t mp3_get_number_of_files_on_tf()
{
    mp3_cmd(0x48, 0, false);
    return mp3_state.number_of_files_on_tf;
}
void mp3_volume_up()
{
    mp3_cmd(0x04, 0, true);
}
void mp3_volume_down()
{
    mp3_cmd(0x05, 0, true);
}
void mp3_play_specified_folder(uint8_t folder_seq, uint8_t file_seq)
{
    mp3_cmd(0x0F, (uint16_t)folder_seq << 8 | (uint16_t)file_seq, true);
}
void mp3_reset()
{
    mp3_cmd(0x0C, 0, false);
}
void mp3_stop()
{
    mp3_cmd(0x16, 0, true);
}
bool mp3_is_idle()
{
    return gpio_get_level(MP3_BUSY) != 0;
}
void mp3_clear_play_completed()
{
    xEventGroupClearBits(mp3_state.event_group, EVENT_PLAY_COMPLETED_BIT);
}
void mp3_wait_for_play_completed(TickType_t xTicksToWait)
{
    xEventGroupWaitBits(mp3_state.event_group, EVENT_PLAY_COMPLETED_BIT, pdFALSE, pdFALSE, xTicksToWait);
}
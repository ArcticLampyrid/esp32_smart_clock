#include "weather.h"
#include <cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <bcd.h>
#include <esp_log.h>
#include <esp_http_client.h>
#define MAX_HTTP_RESPONSE_BUFFER 2048
static char TAG[] = "weather";
weather_info_t g_weather_info = {
    .updated_at = {
        .year = 0x00,
        .month = 0x01,
        .day = 0x01,
        .weekday = WEEKDAY_SATURDAY,
        .hour = 0x00,
        .minute = 0x00,
        .second = 0x00,
    },
};

static bool parse_date_bcd8(const char *str, bcd8_t *year2, bcd8_t *month, bcd8_t *day)
{
    if (str[0] != '2')
        return false;
    if (str[1] != '0')
        return false;
    if (str[2] < '0' || str[2] > '9')
        return false;
    if (str[3] < '0' || str[3] > '9')
        return false;
    if (str[4] != '-')
        return false;
    if (str[5] < '0' || str[5] > '9')
        return false;
    if (str[6] < '0' || str[6] > '9')
        return false;
    if (str[7] != '-')
        return false;
    if (str[8] < '0' || str[8] > '9')
        return false;
    if (str[9] < '0' || str[9] > '9')
        return false;
    if (year2 != NULL)
    {
        *year2 = MAKE_BCD8_HF(str[2] - '0', str[3] - '0');
    }
    if (month != NULL)
    {
        *month = MAKE_BCD8_HF(str[5] - '0', str[6] - '0');
    }
    if (day != NULL)
    {
        *day = MAKE_BCD8_HF(str[8] - '0', str[9] - '0');
    }
    return true;
}

static bool parse_time_bcd8(const char *str, bcd8_t *minute, bcd8_t *hour, bcd8_t *second)
{
    if (str[0] < '0' || str[0] > '9')
        return false;
    if (str[1] < '0' || str[1] > '9')
        return false;
    if (str[2] != ':')
        return false;
    if (str[3] < '0' || str[3] > '9')
        return false;
    if (str[4] < '0' || str[4] > '9')
        return false;
    if (str[5] != ':')
        return false;
    if (str[6] < '0' || str[6] > '9')
        return false;
    if (str[7] < '0' || str[7] > '9')
        return false;
    if (minute != NULL)
    {
        *minute = MAKE_BCD8_HF(str[0] - '0', str[1] - '0');
    }
    if (hour != NULL)
    {
        *hour = MAKE_BCD8_HF(str[3] - '0', str[4] - '0');
    }
    if (second != NULL)
    {
        *second = MAKE_BCD8_HF(str[6] - '0', str[7] - '0');
    }
    return true;
}

static struct rx8025_time_t parse_datetime(const char *str)
{
    struct rx8025_time_t result;
    if (!parse_date_bcd8(str, &result.year, &result.month, &result.day))
    {
        return rx8025_time_min_value();
    }
    if (str[10] != ' ')
    {
        return rx8025_time_min_value();
    }
    if (!parse_time_bcd8(str + 11, &result.hour, &result.minute, &result.second))
    {
        return rx8025_time_min_value();
    }
    return result;
}

void parse_weather_info(weather_info_t *thiz, const char *buffer, size_t length)
{
    cJSON *weather_json = cJSON_ParseWithLength(buffer, length);
    if (weather_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            ESP_LOGE(TAG, "Failed to parse, error before: %s", error_ptr);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse");
        }
        return;
    }
    const cJSON *city = cJSON_GetObjectItemCaseSensitive(weather_json, "city");
    if (cJSON_IsString(city))
        strcpy(thiz->city, city->valuestring);
    else
        thiz->city[0] = '\0';
    const cJSON *data = cJSON_GetObjectItemCaseSensitive(weather_json, "data");
    const cJSON *update_time = cJSON_GetObjectItemCaseSensitive(weather_json, "update_time");
    if (cJSON_IsString(update_time))
        thiz->updated_at = parse_datetime(update_time->valuestring);
    else
        thiz->updated_at = rx8025_time_min_value();
    const cJSON *weather_of_day_json;
    int count = 0;
    cJSON_ArrayForEach(weather_of_day_json, data)
    {
        const cJSON *date = cJSON_GetObjectItemCaseSensitive(weather_of_day_json, "date");
        const cJSON *wea = cJSON_GetObjectItemCaseSensitive(weather_of_day_json, "wea");
        const cJSON *tem_day = cJSON_GetObjectItemCaseSensitive(weather_of_day_json, "tem_day");
        const cJSON *tem_night = cJSON_GetObjectItemCaseSensitive(weather_of_day_json, "tem_night");
        if (cJSON_IsString(wea))
            strcpy(thiz->data[count].weather, wea->valuestring);
        else
            thiz->data[count].weather[0] = '\0';
        if (cJSON_IsString(date))
        {
            parse_date_bcd8(date->valuestring, &thiz->data[count].year, &thiz->data[count].month, &thiz->data[count].day);
        }
        else
        {
            thiz->data[count].year = 0;
            thiz->data[count].month = thiz->data[count].day = 0;
        }
        thiz->data[count].temperature_of_day = cJSON_IsString(tem_day) ? (int8_t)atoi(tem_day->valuestring) : 0;
        thiz->data[count].temperature_of_night = cJSON_IsString(tem_night) ? (int8_t)atoi(tem_night->valuestring) : 0;
        count++;
        if (count >= MAX_WEATHER_DATA_COUNT)
        {
            break;
        }
    }
    thiz->num_of_days = count;
}

void weather_update(weather_info_t *thiz)
{
    char response_buffer[MAX_HTTP_RESPONSE_BUFFER] = {0};
    esp_http_client_config_t config = {
        .url = "http://v0.yiketianqi.com/free/week?unescape=1&appid=14636868&appsecret=UQ8lHDCm",
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0)
    {
        ESP_LOGE(TAG, "HTTP client fetch headers failed");
    }
    else if (content_length > MAX_HTTP_RESPONSE_BUFFER)
    {
        ESP_LOGE(TAG, "HTTP response buffer overflow");
    }
    else
    {
        int data_read = esp_http_client_read_response(client, response_buffer, MAX_HTTP_RESPONSE_BUFFER);
        if (data_read >= 0)
        {
            ESP_LOGI(TAG, "weather response received, length: %d", data_read);
            parse_weather_info(thiz, response_buffer, (size_t)data_read);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read response");
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}
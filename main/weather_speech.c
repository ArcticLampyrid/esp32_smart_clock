#include <string.h>
#include "mp3.h"
#include "weather_speech.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#define MP3_FOLDER_WEATHER 2
#define MP3_WEATHER_SPEECH_TODAY 1
#define MP3_WEATHER_SPEECH_TRANSFORM_TO 2
#define MP3_WEATHER_SPEECH_TEMPERATURE_OF_DAY 3
#define MP3_WEATHER_SPEECH_TEMPERATURE_OF_NIGHT 4
#define MP3_WEATHER_SPEECH_BELOW_ZERO 5
#define MP3_WEATHER_SPEECH_TEMPERATURE_BEGIN 6
#define MP3_WEATHER_SPEECH_BASIC_TEXT_BEGIN 57
static const char *basic_weather_text_map[] = {
    "暴雪",
    "暴雨",
    "暴雨到大暴雨",
    "大暴雨",
    "大暴雨到特大暴雨",
    "大到暴雪",
    "大到暴雨",
    "大雪",
    "大雨",
    "冻雨",
    "多云",
    "浮尘",
    "雷阵雨",
    "雷阵雨伴有冰雹",
    "霾",
    "强沙尘暴",
    "晴",
    "沙尘暴",
    "特大暴雨",
    "雾",
    "小到中雪",
    "小到中雨",
    "小雪",
    "小雨",
    "扬沙",
    "阴",
    "雨夹雪",
    "阵雪",
    "阵雨",
    "中到大雪",
    "中到大雨",
    "中度霾",
    "中雪",
    "中雨",
    "重度霾",
};
static bool string_starts_with(const char *x, const char *y)
{
    return strncmp(x, y, strlen(y)) == 0;
}
static void speak_weather_file_seq(uint8_t seq)
{
    mp3_play_specified_folder(MP3_FOLDER_WEATHER, seq);
    mp3_wait_for_completion();
}
void speak_weather_text(const char *text)
{
    size_t pos = 0;
    while (pos[text] != '\0')
    {
        if (string_starts_with(text, "转"))
        {
            speak_weather_file_seq(MP3_WEATHER_SPEECH_TRANSFORM_TO);
            pos += strlen("转");
            continue;
        }
        bool flag = false;
        for (size_t i = 0; i < sizeof(basic_weather_text_map); i++)
        {
            if (string_starts_with(text + pos, basic_weather_text_map[i]))
            {
                speak_weather_file_seq(MP3_WEATHER_SPEECH_BASIC_TEXT_BEGIN + i);
                pos += strlen(basic_weather_text_map[i]);
                flag = true;
                break;
            }
        }
        if (flag)
        {
            continue;
        }
        pos++;
    }
}
void speak_weather_temperature(int8_t temperature)
{
    if (temperature < 0)
    {
        speak_weather_file_seq(MP3_WEATHER_SPEECH_BELOW_ZERO);
        speak_weather_file_seq(MP3_WEATHER_SPEECH_TEMPERATURE_BEGIN - temperature);
    }
    else
    {
        speak_weather_file_seq(MP3_WEATHER_SPEECH_TEMPERATURE_BEGIN + temperature);
    }
}
void speak_weather_full(const weather_of_day_t *weather_of_day)
{
    speak_weather_file_seq(MP3_WEATHER_SPEECH_TODAY);
    speak_weather_text(weather_of_day->weather);
    speak_weather_file_seq(MP3_WEATHER_SPEECH_TEMPERATURE_OF_DAY);
    speak_weather_temperature(weather_of_day->temperature_of_day);
    speak_weather_file_seq(MP3_WEATHER_SPEECH_TEMPERATURE_OF_NIGHT);
    speak_weather_temperature(weather_of_day->temperature_of_night);
}
static void task_report_weather(void *pvParameters)
{
    weather_update(&g_weather_info);
    speak_weather_full(&g_weather_info.data[0]);
    vTaskDelete(NULL);
}
void update_and_speak_weather_async()
{
    xTaskCreate(task_report_weather, "task_report_weather", 8192, NULL, tskIDLE_PRIORITY, NULL);
}

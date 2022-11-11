#pragma once
#include <stdbool.h>
struct wifi_state_t
{
    bool smart_config_started;
};
struct wifi_state_t wifi_state;
void wifi_init();
void wifi_reset();
void wifi_connect(char *ssid, char *password);
void wifi_connect_by_memory();
#pragma once
#include <stdbool.h>
void ntp_sync_init();
bool ntp_sync_is_enabled();
void ntp_sync_set_enabled(bool enabled);
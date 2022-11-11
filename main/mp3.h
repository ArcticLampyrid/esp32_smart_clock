#pragma once
#include <stdint.h>
#include <stdbool.h>
void mp3_init();
bool mp3_cmd(uint8_t command, uint16_t argument);
uint16_t mp3_get_volume();
uint16_t mp3_get_number_of_files_on_tf();
void mp3_volume_up();
void mp3_volume_down();
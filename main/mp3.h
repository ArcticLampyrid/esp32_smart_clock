#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
void mp3_init();
uint16_t mp3_get_volume();
uint16_t mp3_get_number_of_files_on_tf();
void mp3_volume_up();
void mp3_volume_down();
void mp3_reset();
void mp3_play_specified_folder(uint8_t folder_seq, uint8_t file_seq);
void mp3_stop();
bool mp3_is_idle();
void mp3_clear_play_completed();
void mp3_wait_for_play_completed(TickType_t xTicksToWait);
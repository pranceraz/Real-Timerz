/*
**main header

*/
#pragma once


#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "espnow_example.h"
#include "espnow_example.c"
#include "metronome.h"
#include "structs.h"
#include "input_handler.h"
#include "checker.h"
#include "songs.h"
#include "setup.h"

song_t current_song;

typedef enum {
    SYSTEM_EVENT_SESSION_COMPLETE,
    // Other events if needed
} system_event_t;

//variables
static const char *TAG_MAIN =  "Main";


TaskHandle_t espnow_handle = NULL;
TaskHandle_t pressure_sensor_handle = NULL;
TaskHandle_t metronome_handle = NULL;
TaskHandle_t input_checker_handle = NULL;


void app_main(void);
void Sys_init(void);


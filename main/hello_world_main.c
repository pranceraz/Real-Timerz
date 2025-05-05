/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "input_handler.h"
#include "timer_task.h"
#include "metronome.h"
#include "esp_log.h"
//definitions
#define MAIN_DELAY_MS 5000

//variables
static const char *TAG_MAIN =  "Main";

void Sys_init(void){
   // Initialize hardware
    configure_hardware(); //pressure sensor
}

void app_main(void)
{       

            // --- Basic System Information ---
        // (Optional: Good practice to log chip info on startup)
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        ESP_LOGI(TAG_MAIN, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
                CONFIG_IDF_TARGET,
                chip_info.cores,
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

        ESP_LOGI(TAG_MAIN, "silicon revision %d, ", chip_info.revision);

        uint32_t flash_size;
        esp_flash_get_size(NULL, &flash_size);
        ESP_LOGI(TAG_MAIN, "%" PRIu32 "MB %s flash\n", flash_size / (1024 * 1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

        ESP_LOGI(TAG_MAIN, "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());





        // Initialize hardware
        configure_hardware();
    
        // Create the pressure sensor task
        xTaskCreate(
            pressure_sensor_task,    // Function that implements the task
            "pressure_sensor_task",  // Text name for the task
            2048,                    // Stack size in words
            NULL,                    // Parameter passed into the task
            5,                       // Priority at which the task is created
            NULL                     // Task handle
        );
        
        uint32_t desired_bpm_for_metronome = 90;
        xTaskCreate(
            metronome_task,          // Function that implements the task
            "metronome_task",        // Text name for the task
            2048,                    // Stack size in words
            (void*)desired_bpm_for_metronome,                    // Parameter passed into the task
            5,                       // Priority (same as pressure sensor for now)
            NULL                     // Task handle
        );

        // The scheduler will start automatically
}

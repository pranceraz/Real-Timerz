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
#include "esp_log.h"
//definitions
#define MAIN_DELAY_MS 5000

//variables
static const char *TAG_MAIN =  "Main";

void Sys_init(void){
   // Initialize hardware
    configure_hardware(); //pressure sensore
}

void app_main(void)
{
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
        
        // The scheduler will start automatically
}

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
#include "espnow_receiver.h"
// hi there
void app_main(void)
{
        // Initialize the LED
        init_led();

        // Initialize ESP-NOW and start receiving data
        init_espnow();
    
        while (1) {
            // The receiver is continuously listening, so nothing more is needed in the loop
            vTaskDelay(pdMS_TO_TICKS(1000));  // Just a delay to allow system to run
        }
}

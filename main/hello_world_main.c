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
//#include "espnow_receiver.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "driver/gpio.h"

// Configuration
#define LED_PIN GPIO_NUM_13  // Built-in LED for visual feedback
static const char* TAG = "FPS_RX";

// Structure to match transmitter data format
typedef struct {
    uint8_t state;
} sensor_data_t;

/* ESP-NOW Receive Callback */
// Updated callback signature for ESP-IDF ≥4.4/Arduino-ESP32 ≥3.0
static void espnow_recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (len == sizeof(sensor_data_t)) {
        sensor_data_t *rx_data = (sensor_data_t*)data;
        const uint8_t *mac = info->src_addr;  // Access MAC from structure
        
        gpio_set_level(LED_PIN, rx_data->state ? 1 : 0);
        
        if (rx_data->state != 0) {
            ESP_LOGI(TAG, "RX: [%d%d%d%d] from %02X:%02X:%02X:%02X:%02X:%02X",
                    (rx_data->state >> 3) & 1,
                    (rx_data->state >> 2) & 1,
                    (rx_data->state >> 1) & 1,
                    rx_data->state & 1,
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
    }
}

/* Initialize WiFi and ESP-NOW */
static void init_communication() {
    // 1. Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // 3. Initialize ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
    
    // Optional: Reduce power consumption
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
}

void app_main() {
    // Configure LED
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_conf);
    
    // Initialize communication stack
    init_communication();
    
    ESP_LOGI(TAG, "Receiver ready");
    vTaskDelete(NULL); // FreeRTOS cleanup
}

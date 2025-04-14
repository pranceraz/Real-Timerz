#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "driver/gpio.h"

// Define the LED pin (just as an example of reacting to received data)
#define LED_PIN GPIO_NUM_13           // Built-in LED pin

// Receiver's MAC address (provided)
#define ESP_NOW_PEER_ADDR {0x14, 0x2B, 0x2F, 0xAE, 0xE4, 0x50}

// Function to initialize the LED pin
void init_led() {
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

// Callback function for receiving ESP-NOW data
void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
    // Print the received data
    ESP_LOGI("ESP-NOW", "Received message of length: %d", len);
    
    // If data length is 1 byte, interpret it as a state byte
    if (len == 1) {
        uint8_t received_byte = data[0];
        
        // Display the received byte
        ESP_LOGI("ESP-NOW", "Received byte: 0x%02X", received_byte);
        
        // Control LED based on received byte (example: LED on if any bit is set)
        if (received_byte != 0) {
            gpio_set_level(LED_PIN, 1);  // Turn on LED
        } else {
            gpio_set_level(LED_PIN, 0);  // Turn off LED
        }
    }
}

// ESP-NOW initialization and peer configuration
void init_espnow() {
    // Initialize Wi-Fi in STA mode
    esp_wifi_init(NULL);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        ESP_LOGE("ESP-NOW", "ESP-NOW initialization failed.");
        return;
    }

    // Define the peer (sender's MAC address)
    esp_now_peer_info_t peer_info = {
        .peer_addr = ESP_NOW_PEER_ADDR,
        .channel = 0,
        .encrypt = false,
    };

    // Add the peer (sender) to the peer list
    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        ESP_LOGE("ESP-NOW", "Failed to add peer.");
        return;
    }

    // Register the callback function to receive data
    esp_now_register_recv_cb(espnow_recv_cb);
}
/*
void app_main() {
    // Initialize the LED
    init_led();

    // Initialize ESP-NOW and start receiving data
    init_espnow();

    while (1) {
        // The receiver is continuously listening, so nothing more is needed in the loop
        vTaskDelay(pdMS_TO_TICKS(1000));  // Just a delay to allow system to run
    }
}
*\
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdio.h>

// Configuration Constants
#define FPS1_CHANNEL ADC2_CHANNEL_5   // GPIO12 (bit 3)
#define FPS2_CHANNEL ADC1_CHANNEL_5   // GPIO33 (bit 2)
#define FPS3_CHANNEL ADC2_CHANNEL_3   // GPIO15 (bit 1)
#define FPS4_CHANNEL ADC2_CHANNEL_6   // GPIO14 (bit 0) 

#define LED_PIN GPIO_NUM_13
#define PRESSURE_THRESHOLD 100
#define POLLING_DELAY_MS 50

QueueHandle_t sensor_data_queue = NULL;
static const char* TAG = "FPS_MONITOR";

typedef struct {
    uint8_t state;
} sensor_data_t;

// Helper function to read ADC2
int read_adc2(adc2_channel_t channel) {
    int raw = 0;
    esp_err_t ret = adc2_get_raw(channel, ADC_WIDTH_BIT_12, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC2 channel %d: %s", channel, esp_err_to_name(ret));
        return -1;
    }
    return raw;
}

void configure_hardware() {
    // Configure LED pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(LED_PIN, 0);

    // Configure ADCs
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS2_CHANNEL, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc2_config_channel_atten(FPS1_CHANNEL, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc2_config_channel_atten(FPS3_CHANNEL, ADC_ATTEN_DB_11));
    ESP_ERROR_CHECK(adc2_config_channel_atten(FPS4_CHANNEL, ADC_ATTEN_DB_11));
}

void sensor_reading_task(void *pvParameters) {
    sensor_data_t sensor_data;
    
    while (1) {
        // Read sensors and build state byte (MSB first: GPIO12=bit3, GPIO14=bit0)
        sensor_data.state = 0;
        sensor_data.state |= (read_adc2(FPS1_CHANNEL) > PRESSURE_THRESHOLD) << 3;
        sensor_data.state |= (adc1_get_raw(FPS2_CHANNEL) > PRESSURE_THRESHOLD) << 2;
        sensor_data.state |= (read_adc2(FPS3_CHANNEL) > PRESSURE_THRESHOLD) << 1;
        sensor_data.state |= (read_adc2(FPS4_CHANNEL) > PRESSURE_THRESHOLD) << 0;
        
        if (xQueueSend(sensor_data_queue, &sensor_data, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(TAG, "Queue send failed");
        }
        
        vTaskDelay(pdMS_TO_TICKS(POLLING_DELAY_MS));
    }
}

void display_task(void *pvParameters) {
    sensor_data_t sensor_data;
    static uint8_t prev_state = 0;
    char state_str[10]; // Enough for "[0 0 0 0]"
    
    while (1) {
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY) == pdPASS) {
            // Update LED
            gpio_set_level(LED_PIN, sensor_data.state ? 1 : 0);
            
            // Only print on state change
            if (sensor_data.state != prev_state) {
                if (sensor_data.state != 0) {
                    // Format as [0 1 0 1] style output
                    snprintf(state_str, sizeof(state_str), 
                            "[%d %d %d %d]", 
                            (sensor_data.state >> 3) & 1,
                            (sensor_data.state >> 2) & 1,
                            (sensor_data.state >> 1) & 1,
                            sensor_data.state & 1);
                    ESP_LOGI(TAG, "Pressed: %s", state_str);
                }
                prev_state = sensor_data.state;
            }
        }
    }
}

void app_main() {
    configure_hardware();
    
    sensor_data_queue = xQueueCreate(5, sizeof(sensor_data_t));
    if (!sensor_data_queue) {
        ESP_LOGE(TAG, "Queue creation failed");
        return;
    }

    xTaskCreate(sensor_reading_task, "Sensor Task", 4096, NULL, 3, NULL);
    xTaskCreate(display_task, "Display Task", 4096, NULL, 1, NULL);
    
    vTaskDelete(NULL);
}


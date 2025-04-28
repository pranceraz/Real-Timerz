#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define FPS1_CHANNEL ADC1_CHANNEL_5   // GPIO33
#define FPS2_CHANNEL ADC1_CHANNEL_6   // GPIO34
#define FPS3_CHANNEL ADC1_CHANNEL_0   // GPIO36
#define FPS4_CHANNEL ADC1_CHANNEL_3   // GPIO39
#define LED_PIN GPIO_NUM_13
#define PRESSURE_THRESHOLD 100
#define POLLING_DELAY_MS 50

static const char *TAG = "pressure_sensor";

// Function to process sensor state
void process_sensor_state(uint8_t state) {
    // Log the sensor state in binary format
    char buf[5];
    for (int i = 3; i >= 0; i--) {
        buf[3 - i] = ((state >> i) & 1) ? '1' : '0';
    }
    buf[4] = '\0';
    ESP_LOGI(TAG, "Sensor state: %s", buf);
    
    // Turn on LED if any sensor is pressed
    gpio_set_level(LED_PIN, state ? 1 : 0);
}

// Configure hardware (ADC and GPIO)
static void configure_hardware(void) {
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

    // Configure ADC channels
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS1_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS2_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS3_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS4_CHANNEL, ADC_ATTEN_DB_6));
}

// Pressure sensor reading task
static void pressure_sensor_task(void *pvParameter) {
    uint8_t prev_sensor_state = 0;
    
    // Task runs forever
    for (;;) {
        // Read all sensors and create a 4-bit state
        uint8_t state = 0;
        state |= (adc1_get_raw(FPS1_CHANNEL) > PRESSURE_THRESHOLD) << 3;
        state |= (adc1_get_raw(FPS2_CHANNEL) > PRESSURE_THRESHOLD) << 2;
        state |= (adc1_get_raw(FPS3_CHANNEL) > PRESSURE_THRESHOLD) << 1;
        state |= (adc1_get_raw(FPS4_CHANNEL) > PRESSURE_THRESHOLD) << 0;

        // Only process if state has changed
        if (state != prev_sensor_state) {
            process_sensor_state(state);
            prev_sensor_state = state;
        }
        
        // Wait before next reading
        vTaskDelay(POLLING_DELAY_MS / portTICK_PERIOD_MS);
    }
}

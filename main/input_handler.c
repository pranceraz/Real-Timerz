#include "input_handler.h"
#include "checker.h"

//Function to process sensor state
void process_sensor_state(uint8_t state) {
    // Log the sensor state in binary format
    char buf[5];
    for (int i = 3; i >= 0; i--) {
        buf[3 - i] = ((state >> i) & 1) ? '1' : '0';
    }
    buf[4] = '\0';
    ESP_LOGI(PRESSURE_TAG, "Sensor state: %s", buf);
    
    // Turn on LED if any sensor is pressed
    gpio_set_level(LED_PIN, state ? 1 : 0);
}

// Configure hardware (ADC and GPIO)
void configure_hardware(void) {
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
void pressure_sensor_task(void *pvParameter) {
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
        if (state != prev_sensor_state && state != 0) {
            xQueueSend(input_q, &state, portMAX_DELAY);
            prev_sensor_state = state;
        }
        
        // Wait before next reading
        vTaskDelay(POLLING_DELAY_MS / portTICK_PERIOD_MS);
    }
}

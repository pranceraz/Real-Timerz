#pragma once
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
#define POLLING_DELAY_MS 10

static const char *PRESSURE_TAG = "pressure_sensor";

void process_sensor_state(uint8_t state);
void configure_hardware(void);
void pressure_sensor_task(void *pvParameter);
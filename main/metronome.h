#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include "esp_log.h"

#define METRONOME_BPM 120           // Beats per minute

static const char *TAG_METRONOME = "metronome";
static void metronome_task(void *pvParameter);


// --- Metronome Logic ---
static void metronome_task(void *pvParameter) {
    uint32_t bpm = (uint32_t)pvParameter;

    ESP_LOGI(TAG_METRONOME, "Metronome task started at %lu, BPM.", bpm);
    uint32_t delay_ms = 60000 / bpm; // Calculate delay between beats in milliseconds
    int tick = 0; // To alternate messages


    if (bpm == 0) {
        ESP_LOGI(TAG_METRONOME, "Metronome task received invalid BPM (0). Halting task.");
        vTaskDelete(NULL); // Delete self if BPM is invalid
        return; // Should not be reached after vTaskDelete(NULL)
    }
    for (;;) {
        ESP_LOGI(TAG_METRONOME, "%d",tick);
        
        tick += 1; // Toggle state

        // Delay until the next beat
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

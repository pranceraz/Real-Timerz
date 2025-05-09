#include "metronome.h"
// --- Metronome Logic ---
void metronome_task(void *pvParameter) {

    //xSem = xSemaphoreCreateBinary();
    Metronome_params_t *pParams = (Metronome_params_t *)pvParameter;
    
    uint32_t bpm = pParams->bpm;
    TaskHandle_t espnow_handle = pParams->receiverTaskHandle;


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
        
        tick += 1; // beat count

        // Delay until the next beat
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}
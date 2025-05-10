/*
**metronome.c
*/

#include "metronome.h"
// --- Metronome Logic ---
uint8_t expected_beat_val = 0; 
void metronome_task(void *pvParameter) {

    //xSem = xSemaphoreCreateBinary();
    Metronome_params_t *pParams = (Metronome_params_t *)pvParameter;
    
    uint32_t bpm = pParams->bpm;
    TaskHandle_t metronome_task_handle = pParams->selfTaskHandle;
    song_t selected_song = pParams->song; 

    ESP_LOGI(TAG_METRONOME, "Metronome task started at %lu, BPM.", bpm);
    
    if (bpm == 0) {
        ESP_LOGI(TAG_METRONOME, "Metronome task received invalid BPM (0). Halting task.");
        vTaskDelete(NULL); // Delete self if BPM is invalid
        return; // Should not be reached after vTaskDelete(NULL)
    }
    uint32_t delay_ms = 60000 / bpm; // Calculate delay between beats in milliseconds
    int tick = 0; // To alternate messages


    for (;;) {
        ESP_LOGI(TAG_METRONOME, "%d",tick);
        expected_beat_val = selected_song.beat_values[tick];
        ESP_LOGI(TAG_METRONOME, "expecting %u",expected_beat_val);//debug statement
        
        
        tick += 1; // beat count
        
        // Delay until the next beat
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        if (tick >= selected_song.beat_count){//check if skipping last beat
            tick = 0 ;
            vTaskSuspend(NULL);// replace with starting the setup task


        }
    }
}
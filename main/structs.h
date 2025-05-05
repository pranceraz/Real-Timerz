#pragma once
#include "stdint.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct 
{
    uint32_t bpm;
    TaskHandle_t receiverTaskHandle;
}Metronome_params_t;

struct beat
{
    uint8_t counts;
    uint16_t bpm; 
};

typedef struct song
{
    uint16_t bpm; 
    uint8_t beat_count;         // Number of beats stored
    uint16_t *beat_numbers;  // Keys: up to beat numbers
    uint8_t *beat_values;   //expected  input for each input
}song_t;

extern song_t Songs[3];

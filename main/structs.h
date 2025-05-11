/*structs.h*/

#pragma once
#include "stdint.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>

typedef struct song
{
    uint16_t bpm; 
    uint8_t beat_count;         // Number of beats stored
    uint16_t *beat_numbers;  // Keys: up to beat numbers
    uint8_t *beat_values;   //expected  input for each input
}song_t;

extern song_t Songs[3];

typedef struct 
{
    uint32_t bpm;
    TaskHandle_t selfTaskHandle;
    song_t song;
}Metronome_params_t;


typedef struct 
{
    TaskHandle_t receiverTaskHandle;
    song_t song;
}Checker_params_t;

typedef struct {
    TaskHandle_t espnow_task_handle;
    TaskHandle_t pressure_sensor_task_handle;
    TaskHandle_t metronome_task_handle;
    TaskHandle_t input_checker_task_handle;
    TaskHandle_t self_task_handle;
} Setup_task_params_t;
struct beat //unused
{
    uint8_t counts;
    uint16_t bpm; 
};

extern song_t Songs[3]; 


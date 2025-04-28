#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#define TASK_STACK_SIZE 2048
#define TASK_DELAY_MS 500
//declarations
//public

void metronome(int *input_delay);

//private
static const char *TAG_TASK = "Task";


void metronome(int *input_delay){
    for (;;)
    {
        //metronome task
        vTaskDelay(*input_delay/portTICK_PERIOD_MS);
    }
    
}
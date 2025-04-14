#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#define TASK_STACK_SIZE 2048
#define TASK_DELAY_MS 500
//declarations
//public

void setup_the_task(void);


//private
static const char *TAG_TASK = "Task";
static void the_actual_task(void);

//function definitions
void setup_the_task(void){
    ESP_LOGI(TAG_TASK,"setting up the task");
}
static void the_actual_task(void){
    //
    ;
}
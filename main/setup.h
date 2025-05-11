#pragma once
#include "structs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "espnow_example.h"

static const char *TAG_SETUP = "input_check";

void setup_task(void *pvParameter);
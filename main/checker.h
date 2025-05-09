#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "stdint.h"
#include "structs.h"
static const char *TAG_INPUT = "input_check";

static QueueHandle_t input_q = NULL;
uint8_t latest_input;

void input_checker_task(void *pvParameter);
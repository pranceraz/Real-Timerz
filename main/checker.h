/*
**checker.h
*/
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "stdint.h"
#include "structs.h"
#include "songs.h"
#include "stdbool.h"
#include "metronome.h"
#define INPUT_BUFFER_LENGTH 4

static const char *TAG_INPUT = "input_check";

static QueueHandle_t input_q = NULL;
extern uint8_t latest_input[INPUT_BUFFER_LENGTH];
//extern int input_index; 
extern uint8_t is_correct;
extern bool is_correct_bool;


void input_checker_task(void *pvParameter);
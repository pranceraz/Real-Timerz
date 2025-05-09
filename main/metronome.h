#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <inttypes.h>
#include "esp_log.h"
#include "structs.h"

//#include “semphr.h”

#define METRONOME_BPM 120           // Beats per minute

static const char *TAG_METRONOME = "metronome";

void metronome_task(void *pvParameter);


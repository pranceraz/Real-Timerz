#ifndef METRONOME_H
#define METRONOME_H

#include <stdint.h>
#include "esp_timer.h"
#include <pthread.h>
#include <unistd.h>  // for sleep()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

// External variables (mutex/cond)
extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

// Struct to pass worker arguments
typedef struct {
    uint64_t bpm; // timer period in microseconds
    esp_timer_handle_t metronome; // timer handle
} MetronomeArgs;


// Functions you can call
esp_timer_handle_t create_metronome(void);
void* metronome_worker(void* bpm_ptr);
void* manager_thread(void* arg);

#endif // METRONOME_H

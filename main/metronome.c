#include "metronome.h"
#include "global.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"


static const char* TAG = "Metronome";

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Private function
static void metronome_callback(void* arg);

esp_timer_handle_t create_metronome(void) {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &metronome_callback,
        .name = "metronome"
    };

    esp_timer_handle_t metronome;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &metronome));
    return metronome;
}

void* metronome_worker(void* args) {
    MetronomeArgs* m_args = (MetronomeArgs*)args;

    // Wait for manager to signal to start 
    pthread_mutex_lock(&mutex);
    while (!start && !should_terminate) {
        pthread_cond_wait(&cond, &mutex);
    }

    if (start) {
        ESP_ERROR_CHECK(esp_timer_start_periodic(m_args->metronome, m_args->bpm));
    }
    pthread_mutex_unlock(&mutex);

    // handles the metronone callbacks
    while (1) {
        pthread_mutex_lock(&mutex);

        if (should_terminate) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        pthread_mutex_unlock(&mutex);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_ERROR_CHECK(esp_timer_stop(m_args->metronome));
    ESP_ERROR_CHECK(esp_timer_delete(m_args->metronome));
    ESP_LOGI(TAG, "Worker exited cleanly");

    return NULL;
}

//callback function, put espnow send true/false at end 
static void metronome_callback(void* arg) {
    if (correct_input == 0){
        //hahhshshdshdshdvsd

    }
    else if (current_input == correct_input) {
        scoreboard.hit++;
    } else {
        scoreboard.miss++;
        // Vibrate code here    
    }


    ESP_LOGI(TAG, "Status of input: %d", current_input == correct_input); //prints to serial 1 if match 0 if not 
}

void* manager_thread(void* arg) {
    ESP_LOGI(TAG, "Manager thread started");

    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second
    
    pthread_mutex_lock(&mutex);
    start = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    ESP_LOGI(TAG, "Sent start signal to metronome");

    vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds

    pthread_mutex_lock(&mutex);
    should_terminate = true;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    ESP_LOGI(TAG, "Sent terminate signal to metronome");

    return NULL;
}



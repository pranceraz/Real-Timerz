#include "setup.h"
void setup_task(void *pvParameter) {
    // This task waits for a specific input to "authorize" other tasks to run

    Setup_task_params_t *pParams = (Setup_task_params_t *)pvParameter; 
    TaskHandle_t espnow_handle = pParams->espnow_task_handle;
    TaskHandle_t input_checker_handle = pParams->input_checker_task_handle;
    TaskHandle_t pressure_sensor_handle = pParams->pressure_sensor_task_handle;
    TaskHandle_t metronome_handle = pParams->metronome_task_handle;

    const char *expected_input = "START"; // Example: wait for "START" command
    char received_input[10]; // Buffer to store received input
    bool authorized = false;

    //ESP_LOGI(TAG_SETUP, "Setup task started. Waiting for authorization input: '%s'", expected_input);
    vTaskSuspend(espnow_handle);
    vTaskSuspend(pressure_sensor_handle);
    vTaskSuspend(metronome_handle);
    vTaskSuspend(input_checker_handle);

    ESP_LOGI(TAG_SETUP, "All tasks suspended.");
    
    char command_buf[16];
    if (xQueueReceive(system_control_queue, &command_buf, portMAX_DELAY)== pdPASS) {
        command_buf[15] = '\0';
        ESP_LOGI(TAG_SETUP, "Received command: %s", command_buf);
        if (strcmp(command_buf, "START") == 0) {
            //Once authorized, resume other tasks
            vTaskResume(espnow_handle);
            vTaskResume(pressure_sensor_handle);
            vTaskResume(metronome_handle);
            vTaskResume(input_checker_handle);
            ESP_LOGI(TAG_SETUP, "All tasks resumed.");
        }
    }


    vTaskDelete(NULL); // Delete this task as it's no longer needed
}



//In order for the the tasks to return back to the setup_task you will need a counter passed into the metronome task in order to keep track of how many beats have happened.
//The other tasks will need a way to send the signal back to the setup task.

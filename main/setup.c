/*#include "setup.h"
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
    while (!authorized) {
        // Get input (replace with your actual input method, e.g., UART)
        // For example, using fgets from stdin (requires appropriate setup):
        if (fgets(received_input, sizeof(received_input), stdin) != NULL) {
            //Remove newline character
            received_input[strcspn(received_input, "\n")] = 0;
            ESP_LOGI(TAG_SETUP, "Received input: '%s'", received_input);

            if (strcmp(received_input, expected_input) == 0) {
                authorized = true;
                ESP_LOGI(TAG_SETUP, "Authorization granted!");
            } else {
                ESP_LOGI(TAG_SETUP, "Incorrect input.  Still waiting for '%s'", expected_input);
            }
        } else {
            ESP_LOGE(TAG_SETUP, "Error reading input or no input received.");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before trying again
        }


        if (!authorized) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to prevent starving other tasks if using a busy-wait input method
        }
    }

    //Once authorized, resume other tasks
    vTaskResume(espnow_handle);
    vTaskResume(pressure_sensor_handle);
    vTaskResume(metronome_handle);
    vTaskResume(input_checker_handle);

    ESP_LOGI(TAG_SETUP, "All tasks resumed.");

    vTaskDelete(NULL); // Delete this task as it's no longer needed
}



//In order for the the tasks to return back to the setup_task you will need a counter passed into the metronome task in order to keep track of how many beats have happened.
//The other tasks will need a way to send the signal back to the setup task.
*/
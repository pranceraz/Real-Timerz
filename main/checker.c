#include "checker.h"

uint8_t latest_input;

void input_checker_task(void *pvParameter){
    
    Checker_params_t *pParams = (Checker_params_t *)pvParameter;
    TaskHandle_t espnow_handle = pParams->receiverTaskHandle;

    while (1)
    {
        if (xQueueReceive(input_q, &latest_input, portMAX_DELAY)== pdPASS)//portMAX_DELAY
        {
            //logic to check if input is correct
            
            xTaskNotifyGive(espnow_handle); 

        }
        
    }
    
}
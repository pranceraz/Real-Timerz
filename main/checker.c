/*
**checker.c
*/
#include "checker.h"

uint8_t latest_input[INPUT_BUFFER_LENGTH];//4
int8_t received_value;
uint32_t notification_val_to_send;
int input_index = 0; 
void input_checker_task(void *pvParameter){
    //uint8_t is_correct = 0;
    //bool is_correct_bool = false;
    Checker_params_t *pParams = (Checker_params_t *)pvParameter;
    TaskHandle_t espnow_handle = pParams->receiverTaskHandle;
    input_q = xQueueCreate(5,sizeof(uint8_t));
    while (1)
    {
        if (xQueueReceive(input_q, &latest_input[input_index], portMAX_DELAY)== pdPASS)//portMAX_DELAY
        {
            //logic to check if input is correct            
            //for a value in the latest input buffer
            //check is it matches the expected input
            for (int i = 0; i < 4; i++){

                ESP_LOGI(TAG_METRONOME, "queue has %u at %d",latest_input[i],i);//debug statement

            }
            ESP_LOGI(TAG_INPUT, "Comparing received_value (%u) with expected_beat_val (%u)", latest_input[input_index], expected_beat_val);
            
            
            if(expected_beat_val == latest_input[input_index]){

                for (int i = 0; i < INPUT_BUFFER_LENGTH; i++) {//reset values 
                    latest_input[i] = 0; // Or any other appropriate default value
                    }
                
                notification_val_to_send = 1;
            }
            else
            {
                notification_val_to_send = 0;
            }
            

            input_index++; //increment
            if (input_index >= INPUT_BUFFER_LENGTH)//INPUT INDEX GOES 0,1,2,3
            {                    
                input_index = 0; //reset index
            }
            xTaskNotify(espnow_handle, notification_val_to_send, eSetValueWithOverwrite);
             

        }
        
    }
    
}
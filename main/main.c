/*
**main.c
*/
#include "main.h"

//functions
void Sys_init(void){
   // Initialize hardware
    configure_hardware(); //pressure sensor
    ESP_ERROR_CHECK(nvs_flash_init());
    example_wifi_init();
    example_espnow_init();
    initialize_song_data();
    current_song = Songs[0];
    recv_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    system_control_queue = xQueueCreate(5, sizeof(uint8_t));
    assert(system_control_queue);
    assert(recv_queue);
}

void app_main(void)
{       
        
        Sys_init();
            // --- Basic System Information ---
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        ESP_LOGI(TAG_MAIN, "This is %s chip with %d CPU core(s), WiFi%s%s, ",
                CONFIG_IDF_TARGET,
                chip_info.cores,
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

        ESP_LOGI(TAG_MAIN, "silicon revision %d, ", chip_info.revision);

        uint32_t flash_size;
        esp_flash_get_size(NULL, &flash_size);
        ESP_LOGI(TAG_MAIN, "%" PRIu32 "MB %s flash\n", flash_size / (1024 * 1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

        ESP_LOGI(TAG_MAIN, "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
        
        //order MATTERS
        xTaskCreate(espnow_send_task, "espnow_send_task", 2048, NULL, 4, &espnow_handle);

        // Create the pressure sensor task
        xTaskCreate(
            pressure_sensor_task,    // Function that implements the task
            "pressure_sensor_task",  // Text name for the task
            2048,                    // Stack size in words
            NULL,                    // Parameter passed into the task
            5,                       // Priority at which the task is created
            &pressure_sensor_handle  // Task handle
        );
        
        static Metronome_params_t Metronome_params;
        Metronome_params.bpm = current_song.bpm;
        Metronome_params.selfTaskHandle  = metronome_handle;
        Metronome_params.song = current_song;

        xTaskCreate(
            metronome_task,          // Function that implements the task
            "metronome_task",        // Text name for the task
            2048,                    // Stack size in words
            (void*)&Metronome_params,                    // Parameter passed into the task
            5,                       // Priority (same as pressure sensor for now)
            &metronome_handle                     // Task handle
        );

        static Checker_params_t Checker_params;
        Checker_params.receiverTaskHandle = espnow_handle;
        Checker_params.song = current_song;

        xTaskCreate(
            input_checker_task,
            "input_checker_task",
            2048,
            (void*)&Checker_params,
            6,
            &input_checker_handle
        );

        xTaskCreate(espnow_receive_task, "receive_task", 4096,NULL,3,NULL);
        static Setup_task_params_t setup_params;
        setup_params.espnow_task_handle = espnow_handle;
        setup_params.input_checker_task_handle = input_checker_handle;
        setup_params.metronome_task_handle = metronome_handle;
        setup_params.pressure_sensor_task_handle = pressure_sensor_handle;

        xTaskCreate(setup_task, "setup_task", 4096,(void*)&setup_params, 2,NULL);

        
}

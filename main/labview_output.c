#include "labview_output.h"

QueueHandle_t uart_forward_queue = NULL;


uint8_t s_led_state = 0;

// Define a TAG for this specific module or functionality
// static const char *TAG_BINARY = "BINARY_STATE";

// // Function to log the lower 4 bits of a byte using ESP_LOGI
// void log_4bitbinary_state(uint8_t value) {
//     // Create a buffer to hold the 4 binary digits + null terminator
//     char binary_str[5];

//     // Iterate through the lower 4 bits (3 down to 0)
//     for (int i = 3; i >= 0; i--) {
//         // Calculate the position in the string (bit 3 goes first)
//         int str_pos = 3 - i;
//         // Get the bit (0 or 1) and convert to character '0' or '1'
//         binary_str[str_pos] = ((value >> i) & 1) ? '1' : '0';
//     }
//     // Null-terminate the string
//     binary_str[4] = '\0';

//     // // Log the complete message using ESP_LOGI
//     // ESP_LOGI(TAG_BINARY, "Received state: %s", binary_str);
// }

void receive_esp_inputs_task(void *arg){
    // Check if there's ESP-NOW data to forward to UART (LabVIEW)
    while (1){
        uint8_t state_val;
    if (xQueueReceive(uart_forward_queue, &state_val, 10 / portTICK_PERIOD_MS) == pdTRUE) {
        // char binary_str[5];
        // for (int i = 3; i >= 0; i--) {
        //     binary_str[3 - i] = ((state_val >> i) & 1) ? '1' : '0';
        // }
        // binary_str[4] = '\0';
        if (state_val == 0){
            uart_write_bytes(ECHO_UART_PORT_NUM, "0\r\n", 3);
        }
        if (state_val == 1){
            uart_write_bytes(ECHO_UART_PORT_NUM, "1\r\n", 3);
        }
        }
    }
}

void echo_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1)
    {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len)
        {
            data[len] = '\0';
            if (strncmp((char *)data, "START", 5) == 0) {
                char msg[SONG_MSG_LEN] = {0};
                strncpy(msg, "START", SONG_MSG_LEN - 1);  // Safe copy
                xQueueSend(song_queue, &msg, portMAX_DELAY);
            }
        }
    }
}

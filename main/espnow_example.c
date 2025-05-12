/*
**espnow_example.c 
*/

#include "espnow_example.h"

static const char *TAG = "espnow_sender";
// Helper function to log 4-bit sensor state as binary string
void log_sensor_state(uint8_t state) {
    char buf[5];
    for (int i = 3; i >= 0; i--) {
        buf[3 - i] = ((state >> i) & 1) ? '1' : '0';
    }
    buf[4] = '\0';
    ESP_LOGI(TAG, "Sending: %s", buf);
}



static QueueHandle_t fps_queue = NULL;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = {0, 0};

static example_espnow_send_param_t *send_param = NULL;

// --- ESP-NOW Data Preparation ---
static void example_espnow_data_prepare(example_espnow_send_param_t *send_param, uint8_t sensor_data[4]) {
    example_espnow_data_t *buf = (example_espnow_data_t *)send_param->buffer;
    assert(send_param->len >= sizeof(example_espnow_data_t));
    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? EXAMPLE_ESPNOW_DATA_BROADCAST : EXAMPLE_ESPNOW_DATA_UNICAST;
    buf->state = send_param->state;
    buf->seq_num = s_example_espnow_seq[buf->type]++;
    buf->crc = 0;
    buf->magic = send_param->magic;
    memcpy(buf->sensor_data, sensor_data, sizeof(buf->sensor_data));
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

// --- WiFi and ESPNOW Setup ---
static void example_wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR));
#endif
}

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Optionally log send status
}


#define TAG_RECEIVER_TASK "ESPNOW_reciver"
#define TAG_CB "ESPNOW_RECV_CB" // Specific tag for the callback
#define MAX_ESPNOW_MSG_SIZE 32  

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // if (!recv_info || !data || len <= 0) return;

    // // Copy MAC address and data into a struct for the task
    // example_espnow_event_t evt;
    // example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    // memcpy(recv_cb->mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
    // recv_cb->data = malloc(len);
    // if (!recv_cb->data) return;
    // memcpy(recv_cb->data, data, len);
    // recv_cb->data_len = len;
    // evt.id = EXAMPLE_ESPNOW_RECV_CB;
    // xQueueSend(recv_queue, &evt, 0);

    char message_buffer[MAX_ESPNOW_MSG_SIZE];
    if (recv_info == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG_CB, "Received invalid data (recv_info: %p, data: %p, len: %d)", recv_info, data, len);
        return;
    }
    int copy_len = len;
    if (copy_len >= MAX_ESPNOW_MSG_SIZE) {
        ESP_LOGW(TAG_CB, "Received data length (%d) is too large, truncating to %d bytes.", len, MAX_ESPNOW_MSG_SIZE - 1);
        copy_len = MAX_ESPNOW_MSG_SIZE - 1;
    }
    memcpy(message_buffer, data, copy_len); //copies data to message buffer
    message_buffer[copy_len] = '\0'; 
     if (recv_queue != NULL) {
        if (xQueueSend(recv_queue, &message_buffer, pdMS_TO_TICKS(0)) != pdPASS) {
            // pdMS_TO_TICKS(0) means don't block if the queue is full.
            // You might want a small timeout or handle this error more robustly.
            ESP_LOGW(TAG_CB, "Failed to send message to incoming_espnow_message_queue. Queue full?");
        } else {
            ESP_LOGI(TAG_CB, "Sent to queue: '%s'", message_buffer); // For debugging

        }
    } else {
        ESP_LOGE(TAG_CB, "incoming_espnow_message_queue is NULL! Cannot send message.");
    } // end of null if


    // No need to free `data` here, it's managed by the ESP-NOW stack.
    // We are not `malloc`ing anything in this version of the callback for the message itself.
}


QueueHandle_t system_control_queue = NULL; 
QueueHandle_t recv_queue = NULL;
// --- TASK: ESPNOW Receive ---
void espnow_receive_task(void *pvParameter) {
    // example_espnow_event_t evt;
    // system_control_queue = xQueueCreate(5, sizeof(char[16]));
    // recv_queue = xQueueCreate(5, sizeof(example_espnow_event_t));
    // char control_msg[16];
    // while (1) {
    //     if (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdPASS) {
    //         if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
    //             example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    //             // Ensure null termination
    //             int msg_len = recv_cb->data_len;
    //             if (msg_len >= sizeof(control_msg)) msg_len = sizeof(control_msg) - 1;
    //             memcpy(control_msg, recv_cb->data, msg_len);
    //             control_msg[msg_len] = '\0';

    //             if (strcmp(control_msg, "START") == 0) {
    //                 xQueueSend(system_control_queue, control_msg, 0);
    //                 ESP_LOGI("ESPNOW_RECEIVER", "Received and forwarded START");
    //             }
    //             free(recv_cb->data);
    //         }
    //     }
    // }
    char received_msg_buffer[MAX_ESPNOW_MSG_SIZE];
    ESP_LOGI(TAG_RECEIVER_TASK, "ESP-NOW Receive Processing Task started. Waiting for messages...");
    while (1)
    {
         if (xQueueReceive(recv_queue, &received_msg_buffer, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG_RECEIVER_TASK, "Received message: '%s'", received_msg_buffer);

            // Process the received message
            if (strcmp(received_msg_buffer, "S") == 0) {
                ESP_LOGI(TAG_RECEIVER_TASK, "'START' command received! aka s");

                // If system_control_queue is intended for use by another task:
                if (system_control_queue != NULL) {
                    // Forward the "START" command (or a processed version of it)
                    // For simplicity, sending the same buffer.
                    // Ensure system_control_queue is created to handle MAX_ESPNOW_MSG_SIZE.
                    if (xQueueSend(system_control_queue, received_msg_buffer, pdMS_TO_TICKS(10)) == pdPASS) {
                        ESP_LOGI(TAG_RECEIVER_TASK, "Forwarded 'START' to system_control_queue");
                    } else {
                        ESP_LOGW(TAG_RECEIVER_TASK, "Failed to forward 'START' to system_control_queue (full or timeout)");
                    }
                } else {
                    ESP_LOGI(TAG_RECEIVER_TASK, "system_control_queue is not initialized. 'START' command handled locally if applicable.");
                    // Handle "START" directly in this task if no forwarding is needed
                    // e.g., trigger_start_sequence();
                }

            } else if (strcmp(received_msg_buffer, "STOP") == 0) {
                ESP_LOGI(TAG_RECEIVER_TASK, "'STOP' command received!");
                // Handle STOP command, potentially forward to system_control_queue or act directly
                // Example:
                // if (system_control_queue != NULL) {
                //     xQueueSend(system_control_queue, received_msg_buffer, pdMS_TO_TICKS(10));
                // }

            } else if (strcmp(received_msg_buffer, "RESET_COUNTERS") == 0) {
                ESP_LOGI(TAG_RECEIVER_TASK, "'RESET_COUNTERS' command received!");
                // Handle other specific commands

            } else {
                ESP_LOGI(TAG_RECEIVER_TASK, "Received unknown command: '%s'", received_msg_buffer);
                // Handle or ignore unknown commands as per your application's logic
            }
        }
        // The task will loop back and wait for the next message.
        // A small delay can be added here if the task does very minimal processing
        // and you want to ensure other lower-priority tasks get a chance to run,
        // though `xQueueReceive` with `portMAX_DELAY` is already blocking efficiently.
        // vTaskDelay(pdMS_TO_TICKS(10)); // Usually not needed if xQueueReceive blocks
    
    }
    

}


// --- TASK 2: ESP-NOW Sending Task ---
static void espnow_send_task(void *pvParameter) {
    uint8_t sensor_state;
    uint8_t sensor_data[4];
    //uint32_t notificationValue;
    uint32_t notification_value_received;
    uint8_t value_to_send_over_espnow;

    while (1) {
        //notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        printf("Receiver: Waiting for metronome...\n");
        // notificationValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        

        // if (notificationValue > 0) { 
        //     //log_sensor_state(sensor_state);
        //     esp_now_send(send_param->dest_mac, &is_correct, 1); // Send just one byte
        //     is_correct = 0;
        // }
        if (xTaskNotifyWait(0x00,          /* Don't clear any bits on entry. */
                             ULONG_MAX,     /* Clear all bits on exit (effectively consuming the notification). */
                             &notification_value_received, /* Stores the notification value. */
                             portMAX_DELAY) == pdPASS) {

                                value_to_send_over_espnow = (uint8_t)notification_value_received;
                                ESP_LOGI(TAG, "Notification received by ESP-NOW task. Value: %u", value_to_send_over_espnow);
                             }
        
        esp_now_send(send_param->dest_mac, &value_to_send_over_espnow, sizeof(value_to_send_over_espnow));
    }
}



static esp_err_t example_espnow_init(void) {
    recv_queue = xQueueCreate(5, MAX_ESPNOW_MSG_SIZE);
    system_control_queue = xQueueCreate(5, sizeof(char));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));
    
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (!peer) return ESP_FAIL;
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    send_param = malloc(sizeof(example_espnow_send_param_t));
    if (!send_param) return ESP_FAIL;
    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = 0;
    send_param->delay = 0;
    send_param->len = sizeof(example_espnow_data_t);
    send_param->buffer = malloc(send_param->len);
    if (!send_param->buffer) return ESP_FAIL;
    memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);

    return ESP_OK;
}

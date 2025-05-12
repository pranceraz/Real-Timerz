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
#define TAG_CB "ESPNOW_RECV_CB"
#define MAX_ESPNOW_MSG_SIZE 32

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    // Check if the received data is equal to 8
    if (len == 1 && data[0] == 8) {
        uint8_t received_value = data[0];
        if (recv_queue != NULL) {
            if (xQueueSend(recv_queue, &received_value, pdMS_TO_TICKS(0)) != pdPASS) {
                ESP_LOGW(TAG_CB, "Failed to send value 8 to recv_queue. Queue full?");
            } else {
                ESP_LOGI(TAG_CB, "Sent value 8 to recv_queue");
            }
        } else {
            ESP_LOGE(TAG_CB, "recv_queue is NULL! Cannot send message.");
        }
    } else {
        ESP_LOGI(TAG_CB, "Received data is not 8, ignoring.");
    }
}

QueueHandle_t system_control_queue = NULL;
QueueHandle_t recv_queue = NULL;

void espnow_receive_task(void *pvParameter) {
    uint8_t received_value;
    ESP_LOGI(TAG_RECEIVER_TASK, "ESP-NOW Receive Processing Task started. Waiting for messages...");
    while (1) {
        if (xQueueReceive(recv_queue, &received_value, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG_RECEIVER_TASK, "Received value: '%d'", received_value);

            if (received_value == 8) {
                ESP_LOGI(TAG_RECEIVER_TASK, "Received value is 8, sending to system_control_queue");
                if (system_control_queue != NULL) {
                    if (xQueueSend(system_control_queue, &received_value, pdMS_TO_TICKS(10)) == pdPASS) {
                        ESP_LOGI(TAG_RECEIVER_TASK, "Forwarded value 8 to system_control_queue");
                    } else {
                        ESP_LOGW(TAG_RECEIVER_TASK, "Failed to forward value 8 to system_control_queue (full or timeout)");
                    }
                } else {
                    ESP_LOGE(TAG_RECEIVER_TASK, "system_control_queue is not initialized.");
                }
            } else {
                ESP_LOGI(TAG_RECEIVER_TASK, "Received value is not 8, ignoring.");
            }
        }
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

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


static const char *REC_TAG = "espnow_echo_receiver";
QueueHandle_t recv_queue = NULL;
QueueHandle_t system_control_queue = NULL;
uint8_t state = 0;
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (!recv_info || !data || len <= 0) { 
        return;
    }
    // Copy MAC address and data into a struct for the task
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    memcpy(recv_cb->mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (!recv_cb->data) return;
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    xQueueSend(recv_queue, &evt, 0);
}

// --- TASK: ESPNOW Receive and Echo ---
static void espnow_receive_task(void *pvParameter) {
    example_espnow_event_t evt;
    while (1) {
        if (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
                example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                ESP_LOGI(REC_TAG, "Received %d bytes from "MACSTR, recv_cb->data_len, MAC2STR(recv_cb->mac_addr));
                state = recv_cb->data[0];
                xQueueSend(system_control_queue, &state, 0);
                // Optionally log data
                for (int i = 0; i < recv_cb->data_len; i++) { 
                ESP_LOGI(TAG, "Byte %d: %02X", i, recv_cb->data[i]);
                }
                // Send back an "echo" (single byte = 1) to sender
                free(recv_cb->data);
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

#include "espnow_example.h"
#include "labview_output.h"

#define ESPNOW_QUEUE_SIZE 6 
#define ECHO_BYTE 1
#define CONFIG_ESPNOW_CHANNEL 1

#define ESPNOW_MAXDELAY 512

static const char *TAG = "espnow_echo_receiver";
QueueHandle_t recv_queue = NULL;
QueueHandle_t song_queue = NULL;
uint8_t state = 0b1000;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static example_espnow_send_param_t *send_param = NULL;

static void example_espnow_deinit(example_espnow_send_param_t *send_param); 

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

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // example_espnow_event_t evt;
    // example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    // if (mac_addr == NULL) {
    //     ESP_LOGE(TAG, "Send cb arg error");
    //     return;
    // }

    // evt.id = EXAMPLE_ESPNOW_SEND_CB;
    // memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    // send_cb->status = status;
    // if (xQueueSend(song_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send send queue fail");
    
}

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
static void espnow_echo_task(void *pvParameter) {
    example_espnow_event_t evt;
    while (1) {
        if (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
                example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                //ESP_LOGI(TAG, "Received %d bytes from "MACSTR, recv_cb->data_len, MAC2STR(recv_cb->mac_addr));
                state = recv_cb->data[0];
                xQueueSend(uart_forward_queue, &state, 0);
                // Optionally log data
                // for (int i = 0; i < recv_cb->data_len; i++) { 
                // //ESP_LOGI(TAG, "Byte %d: %02X", i, recv_cb->data[i]);
                // }
                // Send back an "echo" (single byte = 1) to sender
                free(recv_cb->data);
            }
        }
    }
}

// --- TASK 2: ESP-NOW Sending Task ---
static void espnow_send_task(void *pvParameter) {
    uint8_t notify;

    while (1) {
        if (xQueueReceive(song_queue, &notify, portMAX_DELAY) == pdPASS) {
            notify = 7;
            esp_now_send(send_param->dest_mac, &notify, sizeof(notify));

            }
        }
    }

static esp_err_t example_espnow_init(void) {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));

    // Add broadcast peer (so we can receive from any sender)
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

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    example_wifi_init();
    example_espnow_init();

    song_queue = xQueueCreate(10, sizeof(char));
    
    uart_forward_queue = xQueueCreate(5, sizeof(uint8_t));

    recv_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    
    xTaskCreate(receive_esp_inputs_task, "esp_inputs_to_labview", ECHO_TASK_STACK_SIZE, NULL, 3, NULL);

    xTaskCreate(espnow_echo_task, "receive_outside_esp", 2048, NULL, 3, NULL);
    
    xTaskCreate(echo_task, "echo_task", 4096, NULL, 3, NULL);
    xTaskCreate(espnow_send_task, "send_song_outside", 2048, NULL, 3, NULL);
}

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "espnow_example.h"
#include "esp_mac.h"
#include "labview_output.h"

#define ESPNOW_QUEUE_SIZE 6 
#define ECHO_BYTE 1
#define CONFIG_ESPNOW_CHANNEL 1

static const char *TAG = "espnow_echo_receiver";
static QueueHandle_t recv_queue = NULL;
static QueueHandle_t song_queue = NULL;
uint8_t state = 0b1000;

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Optionally log send status
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (!recv_info || !data || len <= 0) return;

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

// Define a TAG for this specific module or functionality
static const char *TAG_BINARY = "BINARY_STATE";

// Function to log the lower 4 bits of a byte using ESP_LOGI
void log_4bitbinary_state(uint8_t value) {
    // Create a buffer to hold the 4 binary digits + null terminator
    char binary_str[5];

    // Iterate through the lower 4 bits (3 down to 0)
    for (int i = 3; i >= 0; i--) {
        // Calculate the position in the string (bit 3 goes first)
        int str_pos = 3 - i;
        // Get the bit (0 or 1) and convert to character '0' or '1'
        binary_str[str_pos] = ((value >> i) & 1) ? '1' : '0';
    }
    // Null-terminate the string
    binary_str[4] = '\0';

    // Log the complete message using ESP_LOGI
    ESP_LOGI(TAG_BINARY, "Received state: %s", binary_str);
}

// --- TASK: ESPNOW Receive and Echo ---
static void espnow_echo_task(void *pvParameter) {
    example_espnow_event_t evt;
    while (1) {
        if (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
                example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                ESP_LOGI(TAG, "Received %d bytes from "MACSTR, recv_cb->data_len, MAC2STR(recv_cb->mac_addr));
                state = recv_cb->data[0];
                log_4bitbinary_state(state);
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
    uint8_t song_choice;

    while (1) {
        if (xQueueReceive(song_queue, &song_choice, portMAX_DELAY) == pdPASS) {
            esp_now_send(send_param->dest_mac, &song_choice, 1); // Send just one byte
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

    // song_queue = xQueueCreate(5, sizeof(uint8_t));
    // assert(song_queue);

    // recv_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    // assert(recv_queue);
    
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(espnow_echo_task, "espnow_echo_task", 2048, NULL, 5, NULL);
    xTaskCreate(espnow_send_task, "espnow_send_song", 2048, NULL, 5, NULL);
}

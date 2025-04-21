#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "espnow_example.h"

#define FPS1_CHANNEL ADC1_CHANNEL_5   // GPIO33 (bit 3)
#define FPS2_CHANNEL ADC1_CHANNEL_6   // GPIO34 (bit 2)
#define FPS3_CHANNEL ADC1_CHANNEL_0   // GPIO36 (bit 1)
#define FPS4_CHANNEL ADC1_CHANNEL_3   // GPIO39 (bit 0)
#define LED_PIN GPIO_NUM_13
#define PRESSURE_THRESHOLD 100
#define POLLING_DELAY_MS 50


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

static void configure_hardware() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(LED_PIN, 0);

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS1_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS2_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS3_CHANNEL, ADC_ATTEN_DB_6));
    ESP_ERROR_CHECK(adc1_config_channel_atten(FPS4_CHANNEL, ADC_ATTEN_DB_6));
}

// --- TASK 1: FPS Reading Task ---
static void fps_read_task(void *pvParameter) {
    uint8_t prev_sensor_state = 0;
    while (1) {
        uint8_t state = 0;
        state |= (adc1_get_raw(FPS1_CHANNEL) > PRESSURE_THRESHOLD) << 3;
        state |= (adc1_get_raw(FPS2_CHANNEL) > PRESSURE_THRESHOLD) << 2;
        state |= (adc1_get_raw(FPS3_CHANNEL) > PRESSURE_THRESHOLD) << 1;
        state |= (adc1_get_raw(FPS4_CHANNEL) > PRESSURE_THRESHOLD) << 0;

        if (state != prev_sensor_state) {
            xQueueSend(fps_queue, &state, 0); // send new state to queue
            prev_sensor_state = state;
        }
        vTaskDelay(POLLING_DELAY_MS / portTICK_PERIOD_MS);
    }
}

// --- TASK 2: ESP-NOW Sending Task ---
static void espnow_send_task(void *pvParameter) {
    uint8_t sensor_state;
    uint8_t sensor_data[4];

    while (1) {
        if (xQueueReceive(fps_queue, &sensor_state, portMAX_DELAY) == pdTRUE) {
            log_sensor_state(sensor_state);
            esp_now_send(send_param->dest_mac, &sensor_state, 1); // Send just one byte
        }
    }
}

// --- TASK 3: LED Blink Task ---
static void led_blink_task(void *pvParameter) {
    uint8_t sensor_state;
    while (1) {
        if (xQueueReceive(fps_queue, &sensor_state, portMAX_DELAY) == pdTRUE) {
            if (sensor_state) {
                gpio_set_level(LED_PIN, 1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                gpio_set_level(LED_PIN, 0);
            }
        }
    }
}

static esp_err_t example_espnow_init(void) {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));

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
    configure_hardware();
    example_wifi_init();
    example_espnow_init();

    fps_queue = xQueueCreate(5, sizeof(uint8_t));
    assert(fps_queue);

    xTaskCreate(fps_read_task,    "fps_read_task",    2048, NULL, 5, NULL);
    xTaskCreate(espnow_send_task, "espnow_send_task", 2048, NULL, 5, NULL);
    xTaskCreate(led_blink_task,   "led_blink_task",   1024, NULL, 4, NULL);
}

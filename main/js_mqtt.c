/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

#include <mqtt_client.h>
#include <esp_log.h>
#include "js_mqtt.h"
#include "js_nvs.h"
#include "js_util.h"
#include "rom/miniz.h"
#include "js_fsm.h"
#include "js_status.h"

static const char *TAG = "js_mqtt.c";
static esp_mqtt_client_handle_t client;
static tdefl_compressor g_deflator;
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            js_on_mqtt_connected();
            break;
        case MQTT_EVENT_DISCONNECTED:
            js_on_mqtt_disconnected();
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            js_on_mqtt_error();
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

void js_mqtt_start() {
    esp_mqtt_client_start(client);
}
void js_mqtt_restart() {
    esp_mqtt_client_stop(client);
    esp_mqtt_client_start(client);
}
void js_mqtt_stop() {
    esp_mqtt_client_stop(client);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    /* The argument passed to esp_mqtt_client_register_event can de accessed as handler_args*/
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void js_miniz_init() {
    int level = 10;
    tdefl_status status;

    static const mz_uint s_tdefl_num_probes[11] = { 0, 1, 6, 32,  16, 32, 128, 256,  512, 768, 1500 };
    mz_uint comp_flags = TDEFL_WRITE_ZLIB_HEADER | s_tdefl_num_probes[level];
    status = tdefl_init(&g_deflator, NULL, NULL, (int)comp_flags);
    if (status != TDEFL_STATUS_OKAY) {
        printf("tdefl_init() failed!\n");
        ESP_ERROR_CHECK( ESP_LOG_ERROR);
    }

}

void js_mqtt_init() {
    const esp_mqtt_client_config_t mqtt_cfg = {
            .reconnect_timeout_ms = 100,
            .uri = js_nvs_mqtt_url(),
            .cert_pem = js_nvs_mqtt_cert(),
            .username = js_nvs_mqtt_user(),
            .password = js_nvs_mqtt_password(),

    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);

}
void js_mqtt_publish_report(uint8_t *message, int len) {
    char mac_str[18];
    char topic_name[32];
    js_mymac_str(mac_str);
    sprintf(topic_name,"JellingStone/%s",mac_str);
    esp_mqtt_client_publish(client, topic_name, (const char *) message, len, 1, 1);
    js_status_ack_sent();
}

void js_mqtt_publish_msg(char *channel, char* message) {
    js_miniz_init();
    size_t message_length = strlen(message);
    char *compressed_message = calloc(1,message_length);
    if(compressed_message == NULL) {
        ESP_LOGE(TAG, "No memory for compressed buffer - available: %d", esp_get_free_heap_size());
        ESP_ERROR_CHECK( ESP_ERR_NO_MEM);
    }
    int cmp_status;
    size_t in_bytes, out_bytes;
    cmp_status = tdefl_compress(&g_deflator, message, &in_bytes, compressed_message, &out_bytes, TDEFL_FINISH);

    if(cmp_status == TDEFL_STATUS_OKAY) {
        ESP_LOGI(TAG, "Compressed %d bytes to %d", in_bytes,out_bytes);
        esp_mqtt_client_publish(client, channel, compressed_message, (int)out_bytes, 1, 1);
    } else {
        ESP_LOGI(TAG, "Failed compressing message - sending uncompressed");
        esp_mqtt_client_publish(client, channel, message, 0, 1, 1);
    }
    free(compressed_message);
}
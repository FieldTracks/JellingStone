/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "util.h"
#include "ntp.h"
#include "status.h"
#include "nvs.h"

#include "lib/zlib/zlib.h"

static const char *TAG = "mqtt.c";

int msgid=0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    //esp_mqtt_client_handle_t client = event->client;
    //int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
             status_set(STATUS_MQTT_CONNECTED);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            status_set(STATUS_MQTT_DISCONNECTED);
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
            break;
	default:
	    ESP_LOGI(TAG, "UNHANDLED_MQTT_EVENT");
	    break;
    }
    return ESP_OK;
}

/*static void mqtt_app_start(void)
{
}
*/
static esp_mqtt_client_handle_t client;
void mqtt_start()
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = get_mqtt_uri(),
        .event_handle = mqtt_event_handler,
        .cert_pem = get_mqtt_cert(),
        .username = get_mqtt_user(),
        .password = get_mqtt_pass(),

        // .user_context = (void *)your_context
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void mqtt_publish(uint8_t mac_id[6], char* message){
    char time_buf[128];
    char topic_name[32];
    char mac_str[18];
    mac2strBLE(mac_id, mac_str);
    time_str(time_buf);
    sprintf(topic_name,"JellingStone/%s",mac_str);

#ifdef CONFIG_MQTT_COMPRESSION
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 11, 5, Z_DEFAULT_STRATEGY);

    unsigned long src_len = strlen(message);
    unsigned long cmp_len = deflateBound(&strm, src_len);

    unsigned char *cmp_buffer = (unsigned char *) malloc(cmp_len);
    if(!cmp_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for compression!");
        return;
    }

    strm.next_in = (unsigned char *) message;
    strm.avail_in = src_len;
    strm.next_out = cmp_buffer;
    strm.avail_out = cmp_len;
    strm.data_type = Z_TEXT;

    int cmp_status = deflate(&strm, Z_FINISH);
    if(cmp_status == Z_STREAM_END) {
        int msg_id = esp_mqtt_client_publish(client, topic_name, (const char *) cmp_buffer, (int) cmp_len, 0, 0);
        ESP_LOGI(TAG, "Sent compressed message via MQTT, ratio=%d%%, msg_id=%d", (int) (((double) src_len / (double) strm.total_out) * 100), msg_id);
        status_ack_sent();
    } else {
        ESP_LOGE(TAG, "Failed to compress message! Error code: %d", cmp_status);
        if(cmp_status == Z_MEM_ERROR) {
            ESP_LOGE(TAG, "=> OUT OF MEMORY, dammit!");
            ESP_LOGE(TAG, "=> Free bytes on heap: %d", esp_get_free_heap_size());
        }
    }

    deflateEnd(&strm);
    free(cmp_buffer);
#else
    int msg_id = esp_mqtt_client_publish(client, topic_name, message, 0, 0, 0);
    status_ack_sent();
#endif
}

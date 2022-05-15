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
#include "js_util.h"

static const char *TAG = "js_mqtt.c";
static esp_mqtt_client_handle_t client;
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

esp_err_t js_mqtt_start() {
    return esp_mqtt_client_start(client);
}
esp_err_t js_mqtt_restart() {
    JS_ERROR_CHECK(esp_mqtt_client_stop(client));
    return esp_mqtt_client_start(client);
}
esp_err_t js_mqtt_stop() {
    return esp_mqtt_client_stop(client);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    /* The argument passed to esp_mqtt_client_register_event can de accessed as handler_args*/
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

esp_err_t js_mqtt_init() {
    const char *user =js_nvs_mqtt_user();
    const char *password =js_nvs_mqtt_password();
    const char *cert =js_nvs_mqtt_cert();
    const char *url = js_nvs_mqtt_url();

    if(user == NULL || password == NULL || cert == NULL || url == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
            .reconnect_timeout_ms = 100,
            .uri = js_nvs_mqtt_url(),
            .cert_pem = js_nvs_mqtt_cert(),
            .username = js_nvs_mqtt_user(),
            .password = js_nvs_mqtt_password(),

    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    if(client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    JS_ERROR_CHECK(esp_mqtt_client_register_event(client,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL));
    return ESP_OK;


}
int js_mqtt_publish_report(uint8_t *message, int len, int *msg_id_out) {
    char mac_str[18];
    char topic_name[37];
    js_mymac_str(mac_str);
    sprintf(topic_name,"JellingStone/%s/scan",mac_str);
    *msg_id_out = esp_mqtt_client_publish(client, topic_name, (const char *) message, len, 1, 1);
    ESP_LOGI(TAG, "Published scan report - size %d bytes", len);
    return ESP_OK;
}

esp_err_t js_mqtt_publish_status(char *message,int *msg_id_out) {
    char mac_str[18];
    char topic_name[39];
    js_mymac_str(mac_str);
    sprintf(topic_name,"JellingStone/%s/status",mac_str);
    size_t len = strlen(message);
    *msg_id_out = esp_mqtt_client_publish(client, topic_name, message, (int) len, 1, 1);
    ESP_LOGI(TAG, "Published status report - size %d bytes", len);

    return ESP_OK;
}

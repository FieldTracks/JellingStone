/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
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
    nvs_handle nvs_handler;
    esp_err_t err = nvs_open("mqtt_config", NVS_READONLY, &nvs_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    size_t size_uri = 0, size_user = 0, size_pass = 0, size_cert = 0;
    esp_err_t err_uri, err_user, err_pass, err_cert;
    err_uri = nvs_get_str(nvs_handler, "uri", NULL, &size_uri);
    err_user = nvs_get_str(nvs_handler, "user", NULL, &size_user);
    err_pass = nvs_get_str(nvs_handler, "pass", NULL, &size_pass);
    err_cert = nvs_get_str(nvs_handler, "cert", NULL, &size_cert);
    if (err_uri != ESP_OK || err_user != ESP_OK || err_pass != ESP_OK || err_cert != ESP_OK){
        ESP_LOGE(TAG, "Error reading values from NVS!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    char *mqtt_uri = (char *)malloc(size_uri);
    char *mqtt_user = (char *)malloc(size_user);
    char *mqtt_pass = (char *)malloc(size_pass);
    char *mqtt_cert = (char *)malloc(size_cert);
    err_uri = nvs_get_str(nvs_handler, "uri", mqtt_uri, &size_uri);
    err_user = nvs_get_str(nvs_handler, "user", mqtt_user, &size_user);
    err_pass = nvs_get_str(nvs_handler, "pass", mqtt_pass, &size_pass);
    err_cert = nvs_get_str(nvs_handler, "cert", mqtt_cert, &size_cert);
    if (err_uri != ESP_OK || err_user != ESP_OK || err_pass != ESP_OK || err_cert != ESP_OK){
        ESP_LOGE(TAG, "Error reading values from NVS!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
      .uri = mqtt_uri,
      .event_handle = mqtt_event_handler,
      .cert_pem = mqtt_cert,
      .username = mqtt_user,
      .password = mqtt_pass,

      // .user_context = (void *)your_context
    };
    nvs_close(nvs_handler);

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void mqtt_publish(uint8_t mac_id[6], char* message){
  char time_buf[128];
  char topic_name[32];
  char mac_str[18];
  mac2str(mac_id, mac_str);
  time_str(time_buf);
  sprintf(topic_name,"/JellingStone/%s",mac_str);

  int msg_id = esp_mqtt_client_publish(client,topic_name, message, 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
  status_ack_sent();


}

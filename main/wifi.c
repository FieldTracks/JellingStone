/*
This file is part of JellingStone - (C) The Fieldtracks Project

    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COLi along with JellingStone.
    If not, please contact info@fieldtracks.org

    Based on esp-idf example code, distributed as public domain and CC0

*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "status.h"

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static const char *TAG = "wifi.c";

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            status_set(STATUS_WIFI_CONNECTED);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            status_set(STATUS_WIFI_DISCONNECTED);
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void start_wifi()
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    
    nvs_handle nvs_handler;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs_handler);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    size_t size_ssid = 0, size_pass = 0;
    esp_err_t err_ssid, err_pass;
    err_ssid = nvs_get_str(nvs_handler, "ssid", NULL, &size_ssid);
    err_pass = nvs_get_str(nvs_handler, "pass", NULL, &size_pass);
    if (err_ssid != ESP_OK || err_pass != ESP_OK){
        ESP_LOGE(TAG, "Error reading values from NVS!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    char *wifi_ssid = (char *)malloc(size_ssid);
    char *wifi_pass = (char *)malloc(size_pass);
    err_ssid = nvs_get_str(nvs_handler, "ssid", wifi_ssid, &size_ssid);
    err_pass = nvs_get_str(nvs_handler, "pass", wifi_pass, &size_pass);
    if (err_ssid != ESP_OK || err_pass != ESP_OK){
        ESP_LOGE(TAG, "Error reading values from NVS!");
        status_set(STATUS_NVS_MISSINGDATA);
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };

    strcpy((char *)wifi_config.sta.ssid, wifi_ssid);
    strcpy((char *)wifi_config.sta.password, wifi_pass);
    nvs_close(nvs_handler);
    free(wifi_ssid);
    free(wifi_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", (char *)wifi_config.sta.ssid, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

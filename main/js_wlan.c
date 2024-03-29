#include <esp_log.h>
#include <esp_event_base.h>
#include <esp_wifi_types.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include "js_nvs.h"
#include "js_wlan.h"
#include "js_ntp.h"
#include "js_fsm.h"
#include "js_util.h"

/*
This file is part of JellingStone - (C) The Fieldtracks Project
    JellingStone is distributed under the civilian open source license (COSLi).
    You should have received a copy of COSLi along with JellingStone.
    If not, please contact info@fieldtracks.org
*/

static char *TAG = "js_wlan";
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t js_wlan_event_group;

static void js_wlan_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "connect to the AP fail - retrying");
        js_on_wlan_disconnected();
    } else if (event_base == WIFI_EVENT) {
        ESP_LOGI(TAG, "Other wifi event %d", event_id);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        js_on_ip_recv();
    }
}

esp_err_t js_wlan_connect() {
    // Reading configuration-values from NVS
    wifi_config_t wifi_config = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK
            },
    };

    JS_ERROR_CHECK(js_nvs_wlan_ssid(wifi_config.sta.ssid));

    JS_ERROR_CHECK(js_nvs_wlan_psk(wifi_config.sta.password));


    ESP_LOGI(TAG, "Connecting to %s", wifi_config.sta.ssid);

    js_wlan_event_group = xEventGroupCreate();

    JS_ERROR_CHECK(esp_netif_init());

    JS_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    JS_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    JS_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &js_wlan_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    JS_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &js_wlan_event_handler,
                                                        NULL,
                                                        &instance_got_ip));


    JS_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    JS_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    JS_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;
}


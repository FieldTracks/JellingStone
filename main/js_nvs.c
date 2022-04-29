#include <esp_log.h>
#include "js_nvs.h"
//
// Created by jan on 24/04/2022.
//
static const char *TAG = "js_nvs";

#define WLAN_SSID_IDX 0
#define WLAN_PSK_IDX 1
#define MQTT_URL_IDX 2
#define MQTT_USER_IDX 3
#define MQTT_PWD_IDX 4
#define BEACON_ID_IDX 5
char *js_nvs_mqtt_data[6];

uint32_t js_nvs_beacon_interval_ms = -1;
uint32_t js_nvs_scan_interval_ms = -1;

uint8_t *js_nvs_wlan_ssid() {
    return (uint8_t *) js_nvs_mqtt_data[WLAN_SSID_IDX];
}

uint8_t *js_nvs_wlan_psk() {
    return (uint8_t *) js_nvs_mqtt_data[WLAN_PSK_IDX];
}

char *js_nvs_mqtt_url() {
    return js_nvs_mqtt_data[MQTT_URL_IDX];
}
char *js_nvs_mqtt_user() {
    return js_nvs_mqtt_data[MQTT_USER_IDX];
}
char *js_nvs_mqtt_pwd() {
    return js_nvs_mqtt_data[MQTT_PWD_IDX];
}
char *js_nvs_beacon_id() {
    return js_nvs_mqtt_data[BEACON_ID_IDX];
}
uint32_t nvs_beacon_interval_ms() {
    return js_nvs_beacon_interval_ms;
}
uint32_t nvs_scan_interval_ms() {
    return js_nvs_scan_interval_ms;
}


void js_nvs_init_value(nvs_handle_t handle, char *name, uint16_t index) {
    size_t required_size;
    esp_err_t err;
    if(js_nvs_mqtt_data[index] != NULL) {
        free(js_nvs_mqtt_data[index]);
    }
    err = nvs_get_str(handle, name, NULL, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading NVS configuration for %s - %s",name, esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }
    js_nvs_mqtt_data[index] = malloc(required_size);
    if(js_nvs_mqtt_data[index] == NULL) {
        err = ESP_ERR_NO_MEM;
        ESP_LOGE(TAG, "Error loading NVS configuration for %s - %s",name, esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }
    err = nvs_get_str(handle, name, js_nvs_mqtt_data[index], &required_size);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Error loading NVS configuration for %s - %s",name, esp_err_to_name(err));
        ESP_ERROR_CHECK(err);
    }

}

void js_nvs_init() {

    ESP_LOGI(TAG,"Loading configuration from NVS");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to init nvs: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK( err );
    }

    nvs_handle_t nvs_handle;
    if((err = nvs_open("config", NVS_READONLY, &nvs_handle)) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening nvs: %s", esp_err_to_name(err));
        ESP_ERROR_CHECK( err );
    }
    js_nvs_init_value(nvs_handle,"WLAN_SSID",WLAN_SSID_IDX);
    js_nvs_init_value(nvs_handle,"WLAN_PSK",WLAN_PSK_IDX);
    js_nvs_init_value(nvs_handle,"MQTT_URL",MQTT_URL_IDX);
    js_nvs_init_value(nvs_handle,"MQTT_USER",MQTT_USER_IDX);
    js_nvs_init_value(nvs_handle,"MQTT_PWD",MQTT_PWD_IDX);
    js_nvs_init_value(nvs_handle,"BEACON_ID",BEACON_ID_IDX);

    if((err = nvs_get_u32(nvs_handle,"BEACON_INT_MS",&js_nvs_beacon_interval_ms)) != ESP_OK) {
        ESP_LOGE(TAG, "Error loading %s: %s", "BEACON_INT_MS", esp_err_to_name(err));
        ESP_ERROR_CHECK( err );
    }
    if((err = nvs_get_u32(nvs_handle,"SCAN_INT_MS",&js_nvs_scan_interval_ms)) != ESP_OK) {
        ESP_LOGE(TAG, "Error loading %s: %s", "SCAN_INT_MS", esp_err_to_name(err));
        ESP_ERROR_CHECK( err );
    }
}


